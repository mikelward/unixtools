/*
 * simple reimplementation of ls
 *
 * TODO
 * - handling of symlink arguments (and -H and -L flags?)
 * - format the size column using the same width for every entry
 * - make -s and -C/-x work together
 * - -l flag
 * - -i flag
 * - -q flag (on by default?) and -b flag
 * - -c flag
 * - -S flag
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
#define _XOPEN_SOURCE 600       /* for strdup() */
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

#include "list.h"
#include "file.h"

/* terminal escape sequences for -G and -K flags
 * not currently customizable
 * see terminfo(3) for details */
typedef struct colors {
    char *black;
    char *red;
    char *green;
    char *yellow;
    char *blue;
    char *white;
    char *none;                     /* escape sequence to go back to default color */
} Colors;

/* all the command line options */
typedef struct options {
    int all : 1;                    /* 1 = also print hidden files */
    int directory : 1;              /* 1 = print directory name rather than contents */
    int dirsonly : 1;               /* 1 = don't print regular files */
    unsigned flags : 2;             /*     print file "flags" */
    int size : 1;                   /* 1 = print file size in blocks */
    int reverse : 1;                /* 0 = forwards, 1 = reverse1 */
    int color : 1;                  /* 1 = colorize file and directory names */
    short blocksize;                /* units for -s option */
    short displaymode;              /* one-per-line, columns, rows, etc. */ 
    short screenwidth;              /* how wide the screen is, 0 if unknown */
    Colors *pcolors;                /* the colors to use */
    file_compare_function compare;  /* determines sort order */
} Options;

enum display { DISPLAY_ONE_PER_LINE, DISPLAY_IN_COLUMNS, DISPLAY_IN_ROWS };
enum flags { FLAGS_NONE, FLAGS_NORMAL, FLAGS_OLD };

int  listfile(File *file, Options *poptions);
void listfilewithnewline(File *file, Options *poptions);
void listfiles(List *files, Options *poptions);
void listdir(File *dir, Options *poptions);
int  setupcolors(Colors *pcolors);
void sortfiles(List *files, Options *poptions);
void usage(void);
int  want(const char *path, Options *poptions);

int main(int argc, char **argv)
{
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
    while ((option = getopt(argc, argv, ":1aCDdFfKkGOstrUx")) != -1) {
        switch(option) {
        case '1':
            options.displaymode = DISPLAY_ONE_PER_LINE;
            break;
        case 'a':
            options.all = 1;
            break;
        case 'C':
            options.displaymode = DISPLAY_IN_COLUMNS;
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
        case 'K':
            /* K = "kolor", somewhat mnemonic and unused in GNU ls */
            options.color = 1;
            break;
        case 'k':
            options.blocksize = 1024;
            break;
        case 'O':
            options.flags = FLAGS_OLD;
            break;
        case 'r':
            options.reverse = 1;
            break;
        case 's':
            options.size = 1;
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
            fprintf(stderr, "Missing argument to -%c\n", optopt);
            usage();
            exit(2);
        case '?':
            fprintf(stderr, "Unknown option -%c\n", optopt);
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
        fprintf(stderr, "ls2: files is NULL\n");
        exit(1);
    }
    List *dirs = newlist();
    if (dirs == NULL) {
        fprintf(stderr, "ls2: dirs is NULL\n");
        exit(1);
    }
    for (int i = 0; i < argc; i++) {
        File *file = newfile(argv[i]);
        if (file == NULL) {
            fprintf(stderr, "ls2: file is NULL\n");
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
            fprintf(stderr, "ls2: dir is NULL\n");
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
 * Print the file.
 *
 * Returns the number of printable characters used,
 * e.g. so that the calling function can pad to the
 * next logical column with spaces.
 */
int listfile(File *file, Options *poptions)
{
    char *name = filename(file);
    if (name == NULL) {
        fprintf(stderr, "ls2: file is NULL\n");
        return 0;
    }

    if (poptions->dirsonly && !isdir(file))
            return 0;

    int nchars = 0;

    if (poptions->size) {
        /* XXX take width into account here */
        unsigned long blocks = getblocks(file, poptions->blocksize);
        printf("%lu ", blocks);
    }

    /*
     * print a character *before* the file showing its type (-O)
     *
     * early versions of UNIX printed directories like [this]
     */
    switch (poptions->flags) {
    case FLAGS_NORMAL:
        break;
    case FLAGS_OLD:
        if (isdir(file))
            nchars += printf("[");
        else if (isexec(file))
            nchars += printf("*");
        break;
    case FLAGS_NONE:
        break;
    }

    /* color the file (-G and -K) */
    /* these escape sequences shouldn't move the cursor,
     * hence we don't increment nchars here */
    /* TODO change this to something like setcolor(COLOR_BLUE) */
    if (poptions->color) {
        if (isdir(file))
            putp(poptions->pcolors->blue);
        else if (isexec(file))
            putp(poptions->pcolors->green);
    }

    /* print the file name */
    nchars += printf("%s", name);

    /* reset the color back to normal (-G) */
    if (poptions->color)
        putp(poptions->pcolors->none);

    /* print a character after the file showing its type (-F and -O) */
    switch (poptions->flags) {
    case FLAGS_NORMAL:
        if (isdir(file))
            nchars += printf("/");
        else if (isexec(file))
            nchars += printf("*");
        else
            nchars += printf(" ");
        break;
    case FLAGS_OLD:
        if (isdir(file))
            nchars += printf("]");
        else if (isexec(file))
            nchars += printf("*");
        else
            nchars += printf(" ");
        break;
    case FLAGS_NONE:
        nchars += printf(" ");
        break;
    }

    free(name);

    return nchars;
}

/**
 * Print the file with a newline.
 *
 * This is needed for one-per-line display mode.
 */
void listfilewithnewline(File *file, Options *poptions)
{
    listfile(file, poptions);
    printf("\n");
}

/**
 * Return the number of characters needed to print this file.
 *
 * Used to calculate the maximum number of characters for all
 * files so we can set up a column with if -C or -x flags were
 * given.
 *
 * TODO Try to share as much of the logic with listfile().
 */
int getfilewidth(void *vfile, void *pvoptions)
{
    File *file = (File *)vfile;
    char *name = filename(file);
    int len = strlen(name);
    free(name);
    return len;
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
        fprintf(stderr, "listfiles: files is NULL\n");
        return;
    } else if (poptions == NULL) {
        fprintf(stderr, "listfiles: poptions is NULL\n");
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
     * ...and finally print the files
     */
    switch (poptions->displaymode) {
    case DISPLAY_ONE_PER_LINE:
        printlist(
            files,
            (printer_func)&listfilewithnewline, poptions);
        break;
    case DISPLAY_IN_COLUMNS:
        printlistdown(
            files,
            poptions->screenwidth, (width_func)&getfilewidth,
            (printer_func)&listfile, poptions);
        break;
    case DISPLAY_IN_ROWS:
        printlistacross(
            files,
            poptions->screenwidth, (width_func)&getfilewidth,
            (printer_func)&listfile, poptions);
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
        fprintf(stderr, "listdir: files in NULL\n");
        return;
    }
    DIR *openeddir = opendir(dir->path);
    if (openeddir == NULL) {
        fprintf(stderr, "listdir: Cannot open %s\n", dir->path);
        return;
    }
    unsigned long totalblocks = 0;
    struct dirent *dirent = NULL;
    while ((dirent = readdir(openeddir)) != NULL) {
        if (!want(dirent->d_name, poptions)) {
            continue;
        }
        File *file = newfile(makepath(dir->path, dirent->d_name));
        if (file == NULL) {
            fprintf(stderr, "listdir: file is NULL\n");
            return;
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

/**
 * Sort "files" based on the specified options.
 */
void sortfiles(List *files, Options *poptions)
{
    if (files == NULL) {
        fprintf(stderr, "sortfiles: files is NULL\n");
        return;
    }
    if (poptions == NULL) {
        fprintf(stderr, "sortfiles: poptions is NULL\n");
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
        fprintf(stderr, "setupcolors: pcolors is NULL\n");
        return 0;
    }

    char *term = getenv("TERM");
    if (term == NULL) {
        return 0;
    }

    int errret;
    if ((setupterm(term, 1, &errret)) == ERR) {
        fprintf(stderr, "setupcolors: setupterm returned %d\n", errret);
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
    fprintf(stderr, "Usage: ls2 [-1aCDdFfGkKOrstUx] <file>...\n");
}

/**
 * Returns true if we should print this file,
 * false otherwise.
 */
int want(const char *path, Options *poptions)
{
    if (path == NULL) {
        fprintf(stderr, "want: path is NULL\n");
        return 0;
    } else if (poptions == NULL) {
        fprintf(stderr, "want: poptions is NULL\n");
        return 0;
    }
    return poptions->all || path[0] != '.';
}

/* vim: set ts=4 sw=4 tw=0 et:*/
