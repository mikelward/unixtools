/*
 * cleaner reimplementation of ls
 *
 * TODO
 * - handling of symlink arguments (and -H and -L flags?)
 * - handling of symlink arguments (-F, link ending in slash, etc.)
 * - correctly calculate column width of extended ("wide") characters
 * - remove remaining statically-sized buffers (search for 1024)
 * - SELinux (and other security systems) support (e.g. "." in modes, -Z flag)
 * - other?
 *
 * NOTES
 * - BSD ls dirtotals are different to mine if using non-standard BLOCKSIZE.
 *   I assume it's the sum of the blocks calculated for each file,
 *   which seems more correct to me, but this decision is not set in stone.
 */

#define _XOPEN_SOURCE 600       /* for strdup(), snprintf() */

#include <sys/types.h>
#include <sys/param.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "buf.h"
#include "display.h"
#include "field.h"
#include "file.h"
#include "filefields.h"
#include "group.h"
#include "list.h"
#include "logging.h"
#include "map.h"
#include "options.h"
#include "user.h"

typedef List FileList;              /* list of files */
typedef List FileFieldList;         /* list of fields for each file */

const int columnmargin = 1;

int  listfile(File *file, Options *options);
void listfilewithnewline(File *file, Options *options);
void listfiles(FileList *files, Options *options);
void listdir(File *dir, Options *options);
void listdirs(FileList *dirs, Options *options, bool firstoutput);
void printtobuf(const char *text, enum escape escape, Buf *buf);
int  printsize(File *file, Options *options);
void printwithnewline(void *string);
void sortfiles(List *files, Options *options);
int  want(File *file, Options *options);

int main(int argc, char **argv)
{
    myname = "l";
    /* so that file names are sorted according to the user's locale */
    setlocale(LC_ALL, "");

    Options *options = newoptions();
    if (!options) {
        error("Out of memory?\n");
        exit(1);
    }

    int nargs = setoptions(options, argc, argv);
    if (nargs == -1) {
        exit(2);
    }

    /* skip program name and flags */ 
    argc -= nargs, argv += nargs;

    /* list current directory if no arguments were given */
    if (argc == 0) {
        argv[0] = ".";
        argc = 1;
    }

    /*
     * ls handles command lines differently depending on
     * whether they are directories or regular files.
     * regular files are listed first, then all directories follow
     */
    List *files = newlist();
    if (!files) {
        error("Out of memory?\n");
        exit(1);
    }
    List *dirs = newlist();
    if (!dirs) {
        error("Out of memory?\n");
        exit(1);
    }
    for (int i = 0; i < argc; i++) {
        File *file = newfile("", argv[i]);
        if (!file) {
            error("Error creating file for %s\n", argv[i]);
            exit(1);
        }
        if (isstat(file)) {
            if (!options->directory && isdir(file)) {
                append(file, dirs);
            } else {
                append(file, files);
            }
        }
    }

    listfiles(files, options);
    freelist(files, (free_func)freefile);

    int nfiles = length(files);
    bool firstoutput = nfiles == 0;
    listdirs(dirs, options, firstoutput);
    freelist(dirs, (free_func)freefile);

    free(options);
}

StringList *makefilestrings(FileFieldList *filefields, int *fieldwidths)
{
    if (filefields == NULL) {
        errorf("filefields is NULL\n");
        return NULL;
    }
    if (fieldwidths == NULL) {
        errorf("fieldwidths is NULL\n");
        return NULL;
    }

    char snprintfbuf[1024];
    StringList *filestrings = newlist();
    if (filestrings == NULL) {
        errorf("filestrings is NULL\n");
        return NULL;
    }

    int nfiles = length(filefields);
    for (int i = 0; i < nfiles; i++) {
        Buf *buf = newbuf();
        if (buf == NULL) {
            errorf("buf is NULL\n");
            freelist(filestrings, (free_func)free);
            return NULL;
        }
        /* XXX could be simpler? */
        List *fields = getitem(filefields, i);
        int nfields = length(fields);
        for (int j = 0; j < nfields; j++) {
            Field *field = getitem(fields, j);
            if (field == NULL) {
                errorf("field is NULL\n");
                freelist(filestrings, (free_func)free);
                freebuf(buf);
                return NULL;
            }
            enum align align = fieldalign(field);
            int screenwidth = fieldwidth(field);
            int paddedwidth = fieldwidths[j];
    
            if (align == ALIGN_RIGHT) {
                for (int k = 0; k < paddedwidth - screenwidth; k++) {
                    bufappend(buf, " ", 1, 1);
                }
            }
            /* always print as many characters as needed */
            int nchars = snprintf(snprintfbuf, sizeof(snprintfbuf),
                "%s", fieldstring(field));
            bufappend(buf, snprintfbuf, nchars, screenwidth);
            if (align == ALIGN_LEFT) {
                for (int k = 0; k < paddedwidth - screenwidth; k++) {
                    bufappend(buf, " ", 1, 1);
                }
            }
            if (j != nfields - 1) {
                for (int k = 0; k < columnmargin; k++) {
                    bufappend(buf, " ", 1, 1);
                }
            }
        }
        append(bufstring(buf), filestrings);
        free(buf);      /* not freebuf because we want to keep the buf->data */
    }

    return filestrings;
}

/**
 * Return an array of the maximum widths of the fields for the given list of list of fields.
 */
int *getmaxfilefieldwidths(FileFieldList *filefields)
{
    if (filefields == NULL) {
        errorf("filefields is NULL\n");
        return NULL;
    }
    int nfiles = length(filefields);
    if (nfiles == 0) {
        return NULL;
    }

    FieldList *firstfilefields = getitem(filefields, 0);
    if (firstfilefields == NULL) {
        errorf("firstfilefields is NULL\n");
        return NULL;
    }
    int nfields = length(firstfilefields);
    int *maxfieldwidths = calloc(nfields+1, sizeof(*maxfieldwidths));
    for (int i = 0; i < nfiles; i++) {
        FieldList *fields = getitem(filefields, i);
        if (fields == NULL) {
            errorf("fields is NULL\n");
            continue;
        }
        for (int j = 0; j < nfields; j++) {
            Field *field = getitem(fields, j);
            int width = fieldwidth(field);
            if (width > maxfieldwidths[j]) {
                maxfieldwidths[j] = width;
            }
        }
    }
    return maxfieldwidths;
}

void printfields(FieldList *fields)
{
    int nfields = length(fields);
    for (int i = 0; i < nfields; i++) {
        Field *field = getitem(fields, i);
        printf("%s ", fieldstring(field));
    }
}

int getfilewidth(int *fieldwidths)
{
    if (fieldwidths == NULL) {
        return 0;
    }

    /* -1 because the loop adds an extra margin even for the first field */
    int total = -columnmargin;
    for (int i = 0; fieldwidths[i] != 0; i++) {
        total += fieldwidths[i] + columnmargin;
    }
    return total;
}

void freefields(List *list)
{
    freelist(list, (free_func)freefield);
}

/**
 * Print the given file list using the specified options.
 *
 * This function does any required sorting and figures out
 * what display format to use for printing.
 */
void listfiles(FileList *files, Options *options)
{
    if (files == NULL) {
        errorf("files is NULL\n");
        return;
    } else if (options == NULL) {
        errorf("options is NULL\n");
        return;
    }

    int nfiles = length(files);
    if (nfiles == 0) {
        return;
    }

    /*
     * sort files according to user preference...
     */
    sortfiles(files, options);

    /*
     * ...reverse the list if -r flag was given...
     * (we used to do this using a custom step in walklist,
     *  but this became overly complicated.
     *  it could also be done in sortfiles(), but I can't
     *  think of a clean way to do that at the moment)
     */
    if (options->reverse) {
        reverselist(files);
    }

    /*
     * ...construct the fields to output for each file...
     */
    FileFieldList *filefields = map(files, (map_func)getfilefields, options);
    int *fieldwidths = getmaxfilefieldwidths(filefields);
    /* we don't own files, so don't free it here */

    /*
     * ...make the output for each file into a single string...
     * (each string being of the same length for use in a columns/rows)
     */
    StringList *filestrings = makefilestrings(filefields, fieldwidths);
    int filewidth = getfilewidth(fieldwidths);
    free(fieldwidths);
    fieldwidths = NULL;
    freelist(filefields, (free_func)freefields);

    /*
     * ...and finally print the output
     */
    switch (options->displaymode) {
    case DISPLAY_ONE_PER_LINE:
        walklist(filestrings, printwithnewline);
        /*printdown(filestrings, 0, 0);*/
        break;
    case DISPLAY_IN_COLUMNS:
        printdown(filestrings, filewidth, options->screenwidth);
        break;
    case DISPLAY_IN_ROWS:
        printacross(filestrings, filewidth, options->screenwidth);
        break;
    }

    freelist(filestrings, (free_func)free);
}

/**
 * Print the contents of the given directory.
 */
void listdir(File *dir, Options *options)
{
    List *files = newlist();
    if (files == NULL) {
        errorf("files in NULL\n");
        return;
    }
    DIR *openeddir = opendir(dir->path);
    if (openeddir == NULL) {
        errorf("Cannot open %s\n", dir->path);
        return;
    }
    unsigned long totalblocks = 0;
    List *subdirs = newlist();
    struct dirent *dirent = NULL;
    while ((dirent = readdir(openeddir)) != NULL) {
        /* TODO: merge this hidden file check with want() */
        if (!options->all && dirent->d_name[0] == '.') {
            continue;
        }
        File *file = newfile(dir->path, dirent->d_name);
        if (file == NULL) {
            errorf("file is NULL\n");
            return;
        }
        if (!want(file, options)) {
            free(file);
            continue;
        }
        append(file, files);
        if (options->recursive && isdir(file)) {
            append(file, subdirs);
        }
        if (options->dirtotals) {
            totalblocks += getblocks(file, options->blocksize);
        }
    }
    closedir(openeddir);

    if (options->dirtotals) {
        printf("total %lu\n", totalblocks);
    }
    listfiles(files, options);
    if (options->recursive) {
        listdirs(subdirs, options, false);
    }
    freelist(files, (free_func)freefile);
}

void listdirs(FileList *dirs, Options *options, bool firstoutput)
{
    if (!dirs) return;
    int ndirs = length(dirs);
    bool needlabel = ndirs > 1 || !firstoutput || options->recursive;
    for (int i = 0; i < ndirs; i++) {
        File *dir = getitem(dirs, i);
        if (!dir) {
            error("dir is NULL\n");
            continue;
        }
        if (i > 0) {
            firstoutput = false;
        }
        if (!firstoutput) {
            printf("\n");
        }
        if (needlabel) {
            printf("%s:\n", dir->path);
        }
        listdir(dir, options);
    }
}

/**
 * Sort "files" based on the specified options.
 */
void sortfiles(List *files, Options *options)
{
    if (files == NULL) {
        errorf("files is NULL\n");
        return;
    }
    if (options == NULL) {
        errorf("options is NULL\n");
        return;
    }

    if (options->compare == NULL) {
        /* user requested no sorting */
        return;
    }

    /* our compare function takes two File **s
     * our sort function says the compare function takes two void **s
     * cast our File ** function to void ** to keep the compiler happy */
    list_compare_function compare = (list_compare_function)options->compare;

    sortlist(files, compare);
}

void printwithnewline(void *string)
{
    puts((char *)string);
}

/**
 * Returns true if we should print this file,
 * false otherwise.
 */
int want(File *file, Options *options)
{
    if (file == NULL) {
        errorf("file is NULL\n");
        return 0;
    } else if (file->path == NULL) {
        errorf("path is NULL\n");
        return 0;
    } else if (options == NULL) {
        errorf("options is NULL\n");
        return 0;
    }
    /* XXX what to do if we can't lstat the file? */
    return !options->dirsonly || isdir(file);
}

/* vim: set ts=4 sw=4 tw=0 et:*/
