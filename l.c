/*
 * cleaner reimplementation of ls
 *
 * TODO
 * - handling of symlink arguments (and -H and -L flags?)
 * - handling of symlink arguments (-F, link ending in slash, etc.)
 * - -S flag
 * - correctly calculate column width of extended ("wide") characters
 * - remove remaining statically-sized buffers (search for 1024)
 * - fix user and group lookups, cache them in a hash
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
#include <ctype.h>
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
#include "list.h"
#include "logging.h"
#include "options.h"

typedef List FileList;              /* list of files */
typedef List FieldList;             /* list of fields for a single file */
typedef List FileFieldList;         /* list of fields for each file */

const int columnmargin = 1;

int  listfile(File *file, Options *options);
void listfilewithnewline(File *file, Options *options);
void listfiles(List *files, Options *options);
void listdir(File *dir, Options *options);
void printnametobuf(const char *name, Options *options, Buf *buf);
int  printsize(File *file, Options *options);
void sortfiles(List *files, Options *options);
void usage(void);
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
        File *file = newfile(".", argv[i]);
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

    int nfiles = length(files);
    listfiles(files, options);
    freelist(files, (free_func)freefile);

    int ndirs = length(dirs);
    int needlabel = nfiles > 0 || ndirs > 1;
    for (int i = 0; i < ndirs; i++) {
        File *dir = getitem(dirs, i);
        if (!dir) {
            error("dir is NULL\n");
            continue;
        }
        if (nfiles > 0 || i > 0) {
            printf("\n");
        }
        if (needlabel) {
            printf("%s:\n", dir->path);
        }
        listdir(dir, options);
    }

    freelist(dirs, (free_func)freefile);
    free(options);
}

void getnamefieldhelper(File *file, Options *options, Buf *buf, int showpath)
{
    assert(file != NULL);
    assert(options != NULL);
    assert(buf != NULL);

    /*
     * print a character *before* the file showing its type (-O)
     *
     * early versions of BSD printed directories like [this]
     */
    switch (options->flags) {
    case FLAGS_NORMAL:
        break;
    case FLAGS_OLD:
        if (!isstat(file))
            bufappend(buf, "?", 1, 1);
        else if (isdir(file))
            bufappend(buf, "[", 1, 1);
        else if (islink(file))
            bufappend(buf, "@", 1, 1);
        else if (isexec(file))
            bufappend(buf, "*", 1, 1);
        break;
    case FLAGS_NONE:
        break;
    }

    /* color the file (-G and -K) */
    /* these escape sequences shouldn't move the cursor,
     * hence the 4th arg to bufappend is 0 */
    /* TODO change this to something like setcolor(COLOR_BLUE) */
    int colorused = 0;
    if (options->color) {
        if (!isstat(file)) {
            bufappend(buf, options->colors->red, strlen(options->colors->red), 0);
            colorused = 1;
        } else if (isdir(file)) {
            bufappend(buf, options->colors->blue, strlen(options->colors->blue), 0);
            colorused = 1;
        } else if (islink(file)) {
            bufappend(buf, options->colors->cyan, strlen(options->colors->cyan), 0);
            colorused = 1;
        } else if (isexec(file)) {
            bufappend(buf, options->colors->green, strlen(options->colors->green), 0);
            colorused = 1;
        }
    }

    const char *name = getname(file);
    printnametobuf(name, options, buf);
    /* don't free name */

    /* reset the color back to normal (-G and -K) */
    if (colorused) {
        bufappend(buf, options->colors->none, strlen(options->colors->none), 0);
    }

    /* print a character after the file showing its type (-F and -O) */
    switch (options->flags) {
    case FLAGS_NORMAL:
        if (!isstat(file))
            bufappend(buf, "?", 1, 1);
        else if (isdir(file))
            bufappend(buf, "/", 1, 1);
        else if (islink(file))
            bufappend(buf, "@", 1, 1);
        else if (isfifo(file))
            bufappend(buf, "|", 1, 1);
        else if (issock(file))
            bufappend(buf, "=", 1, 1);
        else if (isexec(file))
            bufappend(buf, "*", 1, 1);
        else
            bufappend(buf, " ", 1, 1);
        break;
    case FLAGS_OLD:
        if (!isstat(file))
            bufappend(buf, "?", 1, 1);
        else if (isdir(file))
            bufappend(buf, "]", 1, 1);
        else if (islink(file))
            bufappend(buf, "@", 1, 1);
        else if (isexec(file))
            bufappend(buf, "*", 1, 1);
        else
            bufappend(buf, " ", 1, 1);
        break;
    case FLAGS_NONE:
        break;
    }
}

Field *getnamefield(File *file, Options *options)
{
    if (file == NULL) {
        errorf("file is NULL\n");
        return NULL;
    }
    if (options == NULL) {
        errorf("options is NULL\n");
        return NULL;
    }

    Buf *buf = newbuf();
    if (buf == NULL) {
        errorf("buf is NULL\n");
        return NULL;
    }

    /* print the file itself... */
    getnamefieldhelper(file, options, buf, 0);

    if (options->showlinks) {
        /* resolve and print symlink targets recursively */
        while (isstat(file) && islink(file)) {
            file = gettarget(file);
            bufappend(buf, " -> ", 4, 4);
            getnamefieldhelper(file, options, buf, 1);
        }
    } else if (options->showlink) {
        /* print only the first link target without stat'ing the target */
        if (isstat(file) && islink(file)) {
            file = gettarget(file);
            bufappend(buf, " -> ", 4, 4);
            getnamefieldhelper(file, options, buf, 1);
        }
    }

    enum align align = ALIGN_NONE;
    if (options->displaymode != DISPLAY_ONE_PER_LINE) {
        align = ALIGN_LEFT;
    }

    Field *field = newfield(bufstring(buf), align, bufscreenpos(buf));
    if (field == NULL) {
        errorf("field is NULL\n");
        return NULL;
    }
    freebuf(buf);
    return field;
}


/**
 * Returns a list of Fields for the given file.
 *
 * Which fields are returned is controlled by options->
 */
FieldList *getfields(File *file, Options *options)
{
    if (file == NULL) {
        errorf("file is NULL\n");
        return NULL;
    }

    char snprintfbuf[1024];
    List *fieldlist = newlist();
    if (fieldlist == NULL) {
        errorf("fieldlist is NULL\n");
        return NULL;
    }

    /*
     * with -L, display information about the link target file by setting file = target
     * the originally named file is saved as "link" for displaying the name
     */
    File *link = file;
    if (options->targetinfo) {
        File *target = NULL;
        while (isstat(file) && islink(file)) {
            target = gettarget(file);
            if (target == NULL) {
                errorf("target is NULL\n");
                walklist(fieldlist, free);
                free(fieldlist);
                return NULL;
            }
            file = target;
        }
    }

    if (options->size) {
        int width;
        if (isstat(file)) {
            unsigned long blocks = getblocks(file, options->blocksize);
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%lu", blocks);
        } else {
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%s", "?");
        }
        Field *field = newfield(snprintfbuf, ALIGN_RIGHT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (options->inode) {
        int width;
        if (isstat(file)) {
            ino_t inode = getinode(file);
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%lu", inode);
        } else {
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%s", "?");
        }
        Field *field = newfield(snprintfbuf, ALIGN_RIGHT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (options->modes) {
        int width;
        if (isstat(file)) {
            char *modes = getmodes(file);
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%s", modes);
            free(modes);
        } else {
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%s", "???????????");
        }
        Field *field = newfield(snprintfbuf, ALIGN_LEFT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (options->linkcount) {
        int width;
        if (isstat(file)) {
            nlink_t nlinks = getlinkcount(file);
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%lu", (unsigned long)nlinks);
        } else {
             width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%s", "?");
        }
        Field *field = newfield(snprintfbuf, ALIGN_RIGHT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (options->owner) {
        int width;
        if (isstat(file)) {
            if (options->numeric) {
                // TODO error handling
                uid_t uid = getownernum(file);
                width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%lu", (unsigned long)uid);
            } else {
                char *owner = getownername(file);
                width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%s", owner);
            }
        } else {
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "?");
        }
        Field *field = newfield(snprintfbuf, ALIGN_LEFT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (options->group) {
        int width;
        if (isstat(file)) {
            if (options->numeric) {
                // TODO error handling
                gid_t gid = getgroupnum(file);
                width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%lu", (unsigned long)gid);
            } else {
                char *group = getgroupname(file);
                width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%s", group);
            }
        } else {
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "?");
        }
        Field *field = newfield(snprintfbuf, ALIGN_LEFT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (options->perms) {
        char *perms = getperms(file);
        if (perms == NULL) {
            errorf("perms is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        int width = strlen(perms);
        Field *field = newfield(perms, ALIGN_RIGHT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
        free(perms);
    }

    if (options->bytes) {
        int width;
        if (isstat(file)) {
            long bytes = getsize(file);
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%ld", bytes);
        } else {
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "?");
        }
        Field *field = newfield(snprintfbuf, ALIGN_RIGHT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (options->datetime) {
        int width;
        if (isstat(file)) {
            time_t timestamp;
            switch (options->timetype) {
            case TIME_ATIME:
                timestamp = getatime(file);
                break;
            case TIME_CTIME:
                timestamp = getctime(file);
                break;
            /*
            case TIME_BTIME:
                timestamp = getbtime(file);
                break;
            */
            default:
                error("Unknown time attribute\n");
                /* fall through */
            case TIME_MTIME:
                timestamp = getmtime(file);
                break;
            }
            struct tm *timestruct = localtime(&timestamp);
            if (timestruct == NULL) {
                errorf("timestruct is NULL\n");
                walklist(fieldlist, free);
                free(fieldlist);
                return NULL;
            }
            if (options->timeformat != NULL) {
                width = strftime(snprintfbuf, sizeof(snprintfbuf), options->timeformat, timestruct);
            } else {
                /* month day hour and minute if file was modified in the last 6 months,
                   month day year otherwise */
                if (options->now != -1 && timestamp <= options->now && timestamp > options->now - 180*86400) {
                    width = strftime(snprintfbuf, sizeof(snprintfbuf), "%b %e %H:%M", timestruct);
                } else {
                    width = strftime(snprintfbuf, sizeof(snprintfbuf), "%b %e  %Y", timestruct);
                }
            }
        } else {
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "?");
        }
        Field *field = newfield(snprintfbuf, ALIGN_RIGHT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    /*
     * print the file name with flags and colors if requested
     * flags, color start, filename, color end, and flags are made into a single field
     * since we don't want spaces in between
     * if -L is given and the file is a symlink, the returned fields include the name
     * of the link's target (multiple times if there are multiple links)
     * i.e. link -> file
     * or even link -> link -> link -> file
     */
    Field *field = getnamefield(link, options);
    append(field, fieldlist);
    
    return (FieldList *)fieldlist;
}

void printwithnewline(void *string)
{
    puts((char *)string);
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

List *getfields2(File *file, void *pvoptions)
{
    return getfields(file, (Options *)pvoptions);
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
 * Print the given file list using the specified options->
 *
 * This function does any required sorting and figures out
 * what display format to use for printing.
 */
void listfiles(List *files, Options *options)
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
    FileFieldList *filefields = map(files, (map_func)getfields2, options);
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
    struct dirent *dirent = NULL;
    while ((dirent = readdir(openeddir)) != NULL) {
        /* TODO: merge this hidden file check with want() */
        if (!options->all && dirent->d_name[0] == '.') {
            continue;
        }
        File *file = newfile(dir->name, dirent->d_name);
        if (file == NULL) {
            errorf("file is NULL\n");
            return;
        }
        if (!want(file, options)) {
            free(file);
            continue;
        }
        append(file, files);
        if (options->dirtotals) {
            totalblocks += getblocks(file, options->blocksize);
        }
    }
    closedir(openeddir);

    if (options->dirtotals) {
        printf("total %lu\n", totalblocks);
    }
    listfiles(files, options);
    freelist(files, (free_func)freefile);
}

/* XXX does C really not provide this? */
char *cescape(char c)
{
    switch (c) {
    case '\001': return "\\001";
    case '\002': return "\\002";
    case '\003': return "\\003";
    case '\004': return "\\004";
    case '\005': return "\\005";
    case '\006': return "\\006";
    case '\007': return "\\a";
    case '\010': return "\\b";
    case '\011': return "\\t";
    case '\012': return "\\n";
    case '\013': return "\\v";
    case '\014': return "\\f";
    case '\015': return "\\r";
    case '\016': return "\\016";
    case '\017': return "\\017";
    case '\020': return "\\020";
    case '\021': return "\\021";
    case '\022': return "\\022";
    case '\023': return "\\023";
    case '\024': return "\\024";
    case '\025': return "\\025";
    case '\026': return "\\026";
    case '\027': return "\\027";
    case '\030': return "\\030";
    case '\031': return "\\031";
    case '\032': return "\\032";
    case '\033': return "\\033";
    case '\034': return "\\034";
    case '\035': return "\\035";
    case '\036': return "\\036";
    case '\037': return "\\037";
    default: return NULL;
    }
}

void printnametobuf(const char *name, Options *options, Buf *buf)
{
    if (buf == NULL) {
        errorf("buf is NULL\n");
        return;
    }
    if (name == NULL) {
        errorf("file is NULL\n");
        return;
    }

    const char *p = name;
    for (p = name; *p != '\0'; p++) {
        if (!isprint(*p)) {
            switch (options->escape) {
            case ESCAPE_C:
                {
                    char *escaped = cescape(*p);
                    if (escaped == NULL) {
                        errorf("No C escape for %c\n", *p);
                        bufappendchar(buf, *p);
                    } else {
                        bufappend(buf, escaped, strlen(escaped), 1);
                    }
                    break;
                }
            case ESCAPE_QUESTION:
                bufappendchar(buf, '?');
                break;
            default:
                errorf("Unknown escape mode\n");
                /* fall through */
            case ESCAPE_NONE:
                bufappendchar(buf, *p);
                break;
            }
        } else if (*p == '\\' && options->escape == ESCAPE_C) {
            bufappendchar(buf, '\\');
            bufappendchar(buf, '\\');
        } else {
            bufappendchar(buf, *p);
        }
    }
}

/*
int printsize(File *file, Options *options,
              char *buf, size_t bufsize)
{
}
*/

/**
 * Sort "files" based on the specified options->
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
