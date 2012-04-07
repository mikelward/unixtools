/*
 * simple reimplementation of ls
 *
 * TODO
 * - handling of symlink arguments (and -H and -L flags?)
 * - -l flag
 * - -q flag (on by default?) and -b flag
 * - -c flag
 * - -S flag
 * - correctly calculate column width of extended ("wide") characters
 * - remove remaining statically-sized buffers (search for 1024)
 * - other?
 *
 * NOTES
 * - BSD ls dirtotals are different to mine if using non-standard BLOCKSIZE.
 *   I assume it's the sum of the blocks calculated for each file,
 *   which seems more correct to me, but this decision is not set in stone.
 */

/*
 * according to POSIX 2008
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/getopt.html>
 * getopt(), optarg, optind, opterr, and optopt are declared by including
 * <unistd.h> rather than <getopt.h>
 */
#define _XOPEN_SOURCE 600       /* for strdup(), snprintf() */
#define _POSIX_C_SOURCE 200809L /* needed to make getopt() and opt* visible */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <curses.h>
#include <dirent.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <term.h>
#include <unistd.h>

#include "buf.h"
#include "display.h"
#include "field.h"
#include "file.h"
#include "list.h"
#include "logging.h"

/* terminal escape sequences for -G and -K flags
 * not currently customizable
 * see terminfo(3) for details */
typedef struct colors {
    char *black;
    char *red;
    char *green;
    char *yellow;
    char *blue;
    char *magenta;
    char *cyan;
    char *white;
    char *none;                     /* escape sequence to go back to default color */
} Colors;

/* all the command line options */
typedef struct options {
    int all : 1;                    /* 1 = also print hidden files */
    int directory : 1;              /* 1 = print directory name rather than contents */
    int dirsonly : 1;               /* 1 = don't print regular files */
    int inode : 1;                  /* 1 = print the inode number */
    unsigned flags : 2;             /*     print file "flags" */
    int mymodes : 1;                /* display rwx modes for current user */
    int size : 1;                   /* 1 = print file size in blocks */
    int reverse : 1;                /* 0 = forwards, 1 = reverse1 */
    int color : 1;                  /* 1 = colorize file and directory names */
    short blocksize;                /* units for -s option */
    short displaymode;              /* one-per-line, columns, rows, etc. */ 
    short screenwidth;              /* how wide the screen is, 0 if unknown */
    Colors *pcolors;                /* the colors to use */
    file_compare_function compare;  /* determines sort order */
} Options;

typedef List FileList;              /* list of files */
typedef List FieldList;             /* list of fields for a single file */
typedef List FileFieldList;         /* list of fields for each file */
typedef List StringList;            /* list of C strings */

enum display { DISPLAY_ONE_PER_LINE, DISPLAY_IN_COLUMNS, DISPLAY_IN_ROWS };
enum flags { FLAGS_NONE, FLAGS_NORMAL, FLAGS_OLD };

const int columnmargin = 1;

int  listfile(File *file, Options *poptions);
void listfilewithnewline(File *file, Options *poptions);
void listfiles(List *files, Options *poptions);
void listdir(File *dir, Options *poptions);
int  printname(File *file, Options *poptions, char *buf, size_t bufsize);
int  printsize(File *file, Options *poptions);
int  setupcolors(Colors *pcolors);
void sortfiles(List *files, Options *poptions);
void usage(void);
int  want(File *file, Options *poptions);

#define OPTSTRING "1aCDdFfGiKkMOstrUx"

int main(int argc, char **argv)
{
    myname = "l";
    /* so that file names are sorted according to the user's locale */
    setlocale(LC_ALL, "");

    Options options;
    options.all = 0;
    options.blocksize = 1024;
    options.directory = 0;
    options.dirsonly = 0;
    options.displaymode = DISPLAY_ONE_PER_LINE;
    options.color = 0;
    options.flags = FLAGS_NONE;
    options.inode = 0;
    options.mymodes = 0;
    options.size = 0;
    options.reverse = 0;
    options.screenwidth = 0;
    options.compare = &comparebyname;
    options.pcolors = NULL;

    /* use BLOCKSIZE as default blocksize if set */
    char *blocksizeenv = getenv("BLOCKSIZE");
    if (blocksizeenv != NULL) {
        int blocksize = atoi(blocksizeenv);
        if (blocksize != 0) {
            options.blocksize = blocksize;
        }
    }

    /* if output is a terminal, get the terminal's width
     * and turn on the -C flag */
    if (isatty(STDOUT_FILENO)) {
        struct winsize ws;
        if ((ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)) == 0) {
            int width = ws.ws_col;
            if (width != 0) {
                options.screenwidth = width;
                options.displaymode = DISPLAY_IN_COLUMNS;
            }
        }
    }
    /* default width is 80 so we can do columns or rows
     * if the user explicitly specified the -C or -x option
     * (as per BSD and GNU) */
    if (options.screenwidth == 0)
        options.screenwidth = 80;

    opterr = 0;     /* we will print our own error messages */
    int option;
    while ((option = getopt(argc, argv, ":" OPTSTRING)) != -1) {
        switch(option) {
        case '1':
            options.displaymode = DISPLAY_ONE_PER_LINE;
            break;
        case 'a':
            options.all = 1;
            break;
        case 'b':
            /* reserved for escaped control characters mode */
            break;
        case 'C':
            options.displaymode = DISPLAY_IN_COLUMNS;
            break;
        case 'c':
            /* reserved for ctime display */
            break;
        case 'D':
            options.dirsonly = 1;
            break;
        case 'd':
            options.directory = 1;
            break;
        case 'F':
            options.flags = FLAGS_NORMAL;
            break;
        case 'f':
            options.compare = NULL;
            break;
        case 'G':
            /* for compatibility with FreeBSD */
            options.color = 1;
            break;
        case 'g':
            /* reserved for group field */
            break;
        case 'K':
            /* K = "kolor", somewhat mnemonic and unused in GNU ls */
            options.color = 1;
            break;
        case 'k':
            options.blocksize = 1024;
            break;
        case 'i':
            options.inode = 1;
            break;
        case 'l':
            /* reserved for long output mode */
            break;
        case 'M':
            options.mymodes = 1;
            break;
        case 'm':
            /* reserved for modes field (or maybe blocksize=1048576 or stream mode) */
            break;
        case 'N':
            /* possibly reserved for literal control character mode */
            break;
        case 'n':
            /* possibly reserved for numeric owner and group option? */
            break;
        case 'O':
            options.flags = FLAGS_OLD;
            break;
        case 'o':
            /* reserved for owner (user) field */
            break;
        case 'p':
            /* reserved for permissions (modes) field */
            break;
        case 'r':
            options.reverse = 1;
            break;
        case 'S':
            /* possibly reserved for sort by size option */
            break;
        case 's':
            options.size = 1;
            break;
        case 'T':
            /* reserved for time field (display time without -l) */
            break;
        case 't':
            options.compare = &comparebymtime;
            break;
        case 'U':
            options.compare = NULL;
            break;
        case 'x':
            options.displaymode = DISPLAY_IN_ROWS;
            break;
        case ':':
            error("Missing argument to -%c\n", optopt);
            usage();
            exit(2);
        case '?':
            error("Unknown option -%c\n", optopt);
            usage();
            exit(2);
        default:
            usage();
            exit(2);
        }
    }

    /* don't reverse output if it's unsorted
     * (as per BSD and GNU) */
    if (options.compare == NULL) {
        options.reverse = 0;
    }

    Colors colors;
    if (options.color) {
        options.color = setupcolors(&colors);
        options.pcolors = &colors;
    }

    /* skip program name and flags */ 
    argc -= optind, argv += optind;

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
    if (files == NULL) {
        error("files is NULL\n");
        exit(1);
    }
    List *dirs = newlist();
    if (dirs == NULL) {
        error("dirs is NULL\n");
        exit(1);
    }
    for (int i = 0; i < argc; i++) {
        File *file = newfile(argv[i]);
        if (file == NULL) {
            error("file is NULL\n");
            exit(1);
        }
        if (!options.directory && isdir(file)) {
            append(file, dirs);
        } else {
            append(file, files);
        }
    }

    listfiles(files, &options);
    freelist(files);
 
    /*
     * XXX make this use walklist
     */
    int nfiles = length(files);
    int ndirs = length(dirs);
    int needlabel = nfiles > 0 || ndirs > 1;
    for (int i = 0; i < ndirs; i++) {
        File *dir = getitem(dirs, i);
        if (dir == NULL) {
            error("dir is NULL\n");
            continue;
        }
        int neednewline = nfiles > 0 || i > 0;
        if (neednewline) {
            printf("\n");
        }
        if (needlabel) {
            printf("%s:\n", dir->path);
        }
        listdir(dir, &options);
    }

    freelist(dirs);
}

/**
 * Returns a list of Fields for the given file.
 *
 * Which fields are returned is controlled by poptions.
 */
FieldList *getfields(File *file, Options *poptions)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return NULL;
    }

    char snprintfbuf[1024];
    List *fieldlist = newlist();
    if (fieldlist == NULL) {
        errorf(__func__, "fieldlist is NULL\n");
        return NULL;
    }

    if (poptions->inode) {
        ino_t inode = getinode(file);
        long width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%lu", inode);
        Field *field = newfield(snprintfbuf, ALIGN_RIGHT, width);
        if (field == NULL) {
            errorf(__func__, "field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (poptions->size) {
        unsigned long blocks = getblocks(file, poptions->blocksize);
        int width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%lu", blocks);
        Field *field = newfield(snprintfbuf, ALIGN_RIGHT, width);
        if (field == NULL) {
            errorf(__func__, "field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (poptions->mymodes) {
        char *mymodes = getmymodes(file);
        if (mymodes == NULL) {
            errorf(__func__, "mymodes is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        int width = strlen(mymodes);
        Field *field = newfield(mymodes, ALIGN_RIGHT, width);
        if (field == NULL) {
            errorf(__func__, "field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
        free(mymodes);
    }

    /*
     * flags, color start, filename, color end, and flags are made into a single field
     * since we don't want spaces in between
     */

    /*
     * print a character *before* the file showing its type (-O)
     *
     * early versions of BSD printed directories like [this]
     */
    Buf *buf = newbuf();
    if (buf == NULL) {
        errorf(__func__, "buf is NULL\n");
        walklist(fieldlist, (walker_func)freefield);
        freelist(fieldlist);
        return NULL;
    }

    switch (poptions->flags) {
    case FLAGS_NORMAL:
        break;
    case FLAGS_OLD:
        if (isdir(file))
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
    if (poptions->color) {
        if (isdir(file)) {
            bufappend(buf, poptions->pcolors->blue, strlen(poptions->pcolors->blue), 0);
            colorused = 1;
        } else if (islink(file)) {
            bufappend(buf, poptions->pcolors->cyan, strlen(poptions->pcolors->cyan), 0);
            colorused = 1;
        } else if (isexec(file)) {
            bufappend(buf, poptions->pcolors->green, strlen(poptions->pcolors->green), 0);
            colorused = 1;
        }
    }

    /* print the file name */
    int width = printname(file, poptions, snprintfbuf, sizeof(snprintfbuf));
    bufappend(buf, snprintfbuf, width, width);

    /* reset the color back to normal (-G and -K) */
    if (colorused) {
        bufappend(buf, poptions->pcolors->none, strlen(poptions->pcolors->none), 0);
    }

    /* print a character after the file showing its type (-F and -O) */
    switch (poptions->flags) {
    case FLAGS_NORMAL:
        if (isdir(file))
            bufappend(buf, "/", 1, 1);
        else if (islink(file))
            bufappend(buf, "@", 1, 1);
        else if (isexec(file))
            bufappend(buf, "*", 1, 1);
        else
            bufappend(buf, " ", 1, 1);
        break;
    case FLAGS_OLD:
        if (isdir(file))
            bufappend(buf, "]", 1, 1);
        else if (isexec(file))
            bufappend(buf, "*", 1, 1);
        else
            bufappend(buf, " ", 1, 1);
        break;
    }

    enum align align = ALIGN_NONE;
    if (poptions->displaymode != DISPLAY_ONE_PER_LINE) {
        align = ALIGN_LEFT;
    }

    append(newfield(bufdata(buf), align, bufscreenpos(buf)), fieldlist);

    return (FieldList *)fieldlist;
}

void printwithnewline(void *string)
{
    puts((char *)string);
}

StringList *makefilestrings(FileFieldList *filefields, int *fieldwidths)
{
    if (filefields == NULL) {
        errorf(__func__, "filefields is NULL\n");
        return NULL;
    }
    if (fieldwidths == NULL) {
        errorf(__func__, "fieldwidths is NULL\n");
        return NULL;
    }

    char snprintfbuf[1024];
    StringList *filestrings = newlist();
    if (filestrings == NULL) {
        errorf(__func__, "filestrings is NULL\n");
        return NULL;
    }

    int nfiles = length(filefields);
    for (int i = 0; i < nfiles; i++) {
        Buf *buf = newbuf();
        if (buf == NULL) {
            errorf(__func__, "buf is NULL\n");
            walklist(filestrings, free);
            freelist(filestrings);
            return NULL;
        }
        /* XXX could be simpler? */
        List *fields = getitem(filefields, i);
        int nfields = length(fields);
        for (int j = 0; j < nfields; j++) {
            Field *field = getitem(fields, j);
            if (field == NULL) {
                errorf(__func__, "field is NULL\n");
                walklist(filestrings, free);
                freelist(filestrings);
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
        append(bufdata(buf), filestrings);
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
        errorf(__func__, "filefields is NULL\n");
        return NULL;
    }
    int nfiles = length(filefields);
    if (nfiles == 0) {
        return NULL;
    }

    FieldList *firstfilefields = getitem(filefields, 0);
    if (firstfilefields == NULL) {
        errorf(__func__, "firstfilefields is NULL\n");
        return NULL;
    }
    int nfields = length(firstfilefields);
    int *maxfieldwidths = calloc(nfields, sizeof(*maxfieldwidths)+1);
    for (int i = 0; i < nfiles; i++) {
        FieldList *fields = getitem(filefields, i);
        if (fields == NULL) {
            errorf(__func__, "fields is NULL\n");
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

/**
 * Print the given file list using the specified options.
 *
 * This function does any required sorting and figures out
 * what display format to use for printing.
 */
void listfiles(List *files, Options *poptions)
{
    if (files == NULL) {
        errorf(__func__, "files is NULL\n");
        return;
    } else if (poptions == NULL) {
        errorf(__func__, "poptions is NULL\n");
        return;
    }

    int nfiles = length(files);
    if (nfiles == 0) {
        return;
    }

    /*
     * sort files according to user preference...
     */
    sortfiles(files, poptions);

    /*
     * ...reverse the list if -r flag was given...
     * (we used to do this using a custom step in walklist,
     *  but this became overly complicated.
     *  it could also be done in sortfiles(), but I can't
     *  think of a clean way to do that at the moment)
     */
    if (poptions->reverse) {
        reverselist(files);
    }

    /*
     * ...construct the fields to output for each file...
     */
    FileFieldList *filefields = map(files, (map_func)getfields2, poptions);
    int *fieldwidths = getmaxfilefieldwidths(filefields);

    /*
     * ...make the output for each file into a single string...
     * (each string being of the same length for use in a columns/rows)
     */
    StringList *filestrings = makefilestrings(filefields, fieldwidths);
    int filewidth = getfilewidth(fieldwidths);

    /*
     * ...and finally print the output
     */
    switch (poptions->displaymode) {
    case DISPLAY_ONE_PER_LINE:
        walklist(filestrings, printwithnewline);
        /*printdown(filestrings, 0, 0);*/
        break;
    case DISPLAY_IN_COLUMNS:
        printdown(filestrings, filewidth, poptions->screenwidth);
        break;
    case DISPLAY_IN_ROWS:
        printacross(filestrings, filewidth, poptions->screenwidth);
        break;
    }
}

/**
 * Print the contents of the given directory.
 */
void listdir(File *dir, Options *poptions)
{
    List *files = newlist();
    if (files == NULL) {
        errorf(__func__, "files in NULL\n");
        return;
    }
    DIR *openeddir = opendir(dir->path);
    if (openeddir == NULL) {
        errorf(__func__, "Cannot open %s\n", dir->path);
        return;
    }
    unsigned long totalblocks = 0;
    struct dirent *dirent = NULL;
    while ((dirent = readdir(openeddir)) != NULL) {
        /* TODO: merge this hidden file check with want() */
        if (!poptions->all && dirent->d_name[0] == '.') {
            continue;
        }
        File *file = newfile(makepath(dir->path, dirent->d_name));
        if (file == NULL) {
            errorf(__func__, "file is NULL\n");
            return;
        }
        if (!want(file, poptions)) {
            continue;
        }
        append(file, files);
        if (poptions->size) {
            totalblocks += getblocks(file, poptions->blocksize);
        }
    }

    if (poptions->size) {
        printf("total %lu\n", totalblocks);
    }
    listfiles(files, poptions);
}

int printname(File *file, Options *poptions,
              char *buf, size_t bufsize)
{
    if (buf == NULL) {
        errorf(__func__, "buf is NULL\n");
        return 0;
    }
    char *name = filename(file);
    if (name == NULL) {
        errorf(__func__, "file is NULL\n");
        *buf = '\0';
        return 0;
    }

    int width = snprintf(buf, bufsize, "%s", name);
    free(name);
    return width;
}

/*
int printsize(File *file, Options *poptions,
              char *buf, size_t bufsize)
{
}
*/

/**
 * Sort "files" based on the specified options.
 */
void sortfiles(List *files, Options *poptions)
{
    if (files == NULL) {
        errorf(__func__, "files is NULL\n");
        return;
    }
    if (poptions == NULL) {
        errorf(__func__, "poptions is NULL\n");
        return;
    }

    if (poptions->compare == NULL) {
        /* user requested no sorting */
        return;
    }

    /* our compare function takes two File **s
     * our sort function says the compare function takes two void **s
     * cast our File ** function to void ** to keep the compiler happy */
    list_compare_function compare = (list_compare_function)poptions->compare;

    sortlist(files, compare);
}

/**
 * Try to set up color output.
 *
 * Returns 1 on success, 0 on failure.
 *
 * TODO This could be cleaned up a bit,
 * perhaps use an array for the colors, etc.
 * And maybe there's a way to get this
 * more simply.
 */
int setupcolors(Colors *pcolors)
{
    if (pcolors == NULL) {
        errorf(__func__, "pcolors is NULL\n");
        return 0;
    }

    char *term = getenv("TERM");
    if (term == NULL) {
        return 0;
    }

    int errret;
    if ((setupterm(term, 1, &errret)) == ERR) {
        errorf(__func__, "setupterm returned %d\n", errret);
        return 0;
    }

    char *setaf = tigetstr("setaf");
    if (setaf == NULL) {
        return 0;
    }
    pcolors->black = strdup(tparm(setaf, COLOR_BLACK, NULL));
    pcolors->red = strdup(tparm(setaf, COLOR_RED, NULL));
    pcolors->green = strdup(tparm(setaf, COLOR_GREEN, NULL));
    pcolors->yellow = strdup(tparm(setaf, COLOR_YELLOW, NULL));
    pcolors->blue = strdup(tparm(setaf, COLOR_BLUE, NULL));
    pcolors->magenta = strdup(tparm(setaf, COLOR_MAGENTA, NULL));
    pcolors->cyan = strdup(tparm(setaf, COLOR_CYAN, NULL));
    pcolors->white = strdup(tparm(setaf, COLOR_WHITE, NULL));
    char *sgr0 = tigetstr("sgr0");
    if (sgr0 == NULL) {
        return 0;
    }
    pcolors->none = strdup(sgr0);

    return 1;
}

/**
 * Print the help message.
 */
void usage(void)
{
    fprintf(stderr, "Usage: %s: %s: <file>...\n", myname, OPTSTRING);
}

/**
 * Returns true if we should print this file,
 * false otherwise.
 */
int want(File *file, Options *poptions)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return 0;
    } else if (file->path == NULL) {
        errorf(__func__, "path is NULL\n");
        return 0;
    } else if (poptions == NULL) {
        errorf(__func__, "poptions is NULL\n");
        return 0;
    }
    return !poptions->dirsonly || isdir(file);
}

/* vim: set ts=4 sw=4 tw=0 et:*/
