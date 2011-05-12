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
#include <sys/stat.h>
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

typedef struct colors {
    char *black;
    char *red;
    char *green;
    char *yellow;
    char *blue;
    char *white;
    char *none;
} Colors;

/* all the command line options */
typedef struct options {
    int all : 1;
    int directory : 1;
    int dirsonly : 1;
    unsigned flags : 2;
    int size : 1;
    int reverse : 1;                /* forwards = 0, reverse = 1 */
    int color : 1;                  /* whether to use color */
    short blocksize;
    short displaymode;              /* one-per-line, columns, rows, etc. */ 
    short screenwidth;              /* how wide the screen is, 0 if unknown */
    Colors *pcolors;                /* the colors to use */
    file_compare_function compare;
} Options;

enum display { DISPLAY_ONE_PER_LINE, DISPLAY_IN_COLUMNS, DISPLAY_IN_ROWS };
enum flags { FLAGS_NONE, FLAGS_NORMAL, FLAGS_OLD };

/* getblocks is here rather than file because
 * user options may change the units */
unsigned long getblocks(File *file, Options *poptions);
int  listfile(File *file, Options *poptions);
void listfilewithnewline(File *file, Options *poptions);
void listfiles(List *files, Options *poptions);
void listdir(File *dir, Options *poptions);
int setupcolors(Colors *pcolors);
void sortfiles(List *files, Options *poptions);
void usage(void);
int want(const char *path, Options *poptions);

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

    /* don't reverse output if it's unsorted */
    /* (as per BSD and GNU) */
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
        unsigned long blocks = getblocks(file, poptions);
        printf("%lu ", blocks);
    }

    /*
     * print a character that shows the file's type (-F)
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

    /* color the file (-G) */
    /* don't increment column here because the colors shouldn't
     * move the cursor */
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

    /* print a character that shows the file's type (-F) */
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

/*
 * for one-per-line mode
 */
void listfilewithnewline(File *file, Options *poptions)
{
    listfile(file, poptions);
    printf("\n");
}

int getfilewidth(void *vfile, void *pvoptions)
{
    File *file = (File *)vfile;
    char *name = filename(file);
    int len = strlen(name);
    free(name);
    return len;
}

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
     * much easier to reverse the list
     * than make printlist* functions much more complex
     */
    if (poptions->reverse) {
        /* TODO reverse list here */
    }

    /*
     * ...then print them
     */
    switch (poptions->displaymode) {
    case DISPLAY_ONE_PER_LINE:
        printlist(files, (printer_func)&listfilewithnewline, poptions);
        break;
    case DISPLAY_IN_COLUMNS:
        printlistdown(files, poptions->screenwidth, (width_func)&getfilewidth, (printer_func)&listfile, poptions);
        break;
    case DISPLAY_IN_ROWS:
        printlistacross(files, poptions->screenwidth, (width_func)&getfilewidth, (printer_func)&listfile, poptions);
        break;
    }
}

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
            totalblocks += getblocks(file, poptions);
        }
    }

    if (poptions->size) {
        printf("total %lu\n", totalblocks);
    }
    listfiles(files, poptions);
}

unsigned long getblocks(File *file, Options *poptions)
{
    struct stat *pstat = getstat(file);
    if (pstat == NULL) {
        fprintf(stderr, "getblocks: pstat is NULL\n");
        return 0;
    }
    if (poptions == NULL) {
        fprintf(stderr, "getblocks: poptions is NULL\n");
        return 0;
    }
    unsigned long blocks = pstat->st_blocks;
    /* blocks are stored as an unsigned long on i686
     * when dealing with integral types, we have to do
     * larger / smaller to avoid getting zero for everything
     * but we also want to do the blocksize/BSIZE calculation
     * first to reduce the chances of overflow */
    if (poptions->blocksize > DEV_BSIZE) {
        int factor = poptions->blocksize / DEV_BSIZE;
        /* round up to nearest integer
         * e.g. if it takes 3 * 512 byte blocks,
         * it would take 2 1024 byte blocks */
        return (blocks+(factor/2)) / factor;
    }
    else {
        int factor = DEV_BSIZE / poptions->blocksize;
        return blocks * factor;
    }
}

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

void usage(void)
{
    fprintf(stderr, "Usage: ls2 [-1aCDdFfGkKOrstUx] <file>...\n");
}

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
