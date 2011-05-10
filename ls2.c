/*
 * simple reimplementation of ls
 *
 * TODO
 * - most flags
 * - handling of symlink arguments
 */

/*
 * according to POSIX 2008
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/getopt.html>
 * getopt(), optarg, optind, opterr, and optopt are declared by including
 * <unistd.h> rather than <getopt.h>
 */
#define _POSIX_C_SOURCE 200809L /* needed to make getopt() and opt* visible */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
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
    int flags : 1;
    int size : 1;
    int step : 2;                   /* forwards = 1, reverse = -1 */
    int color : 1;                  /* whether to use color */
    short blocksize;
    short displaymode;
    short width;                    /* how wide the screen is, 0 if unknown */
    Colors *pcolors;                /* the colors to use */
    file_compare_function compare;
} Options;

typedef struct file_data {
    int maxwidth;
} FileData;

enum display { DISPLAY_ONE_PER_LINE, DISPLAY_IN_COLUMNS };

/* getblocks is here rather than file because
 * user options may change the units */
unsigned long getblocks(File *file, Options *poptions);
void listfile(File *file, Options *poptions);
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
    options.flags = 0;
    options.size = 0;
    options.step = 1;
    options.width = 0;
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

    /* turn -C on by default if output is a terminal */
    char *columnsenv = getenv("COLUMNS");
    if (columnsenv != NULL) {
        int width = atoi(columnsenv);
        if (width != 0) {
            options.width = width;
            options.displaymode = DISPLAY_IN_COLUMNS;
        }
    }

    opterr = 0;     /* we will print our own error messages */
    int option;
    while ((option = getopt(argc, argv, ":1aCDdFfkGstrU")) != -1) {
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
            options.flags = 1;
            break;
        case 'f':
            options.compare = NULL;
            break;
        case 'G':
            options.color = 1;
            break;
        case 'k':
            options.blocksize = 1024;
            break;
        case 'r':
            options.step = -1;
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

    /* it doesn't make sense to reverse unsorted output
     * follow GNU ls and ignore -r */
    if (options.compare == NULL) {
        options.step = 1;
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

void listfile(File *file, Options *poptions)
{
    char *name = filename(file);
    if (name == NULL) {
        fprintf(stderr, "ls2: file is NULL\n");
        return;
    }

    if (poptions->dirsonly && !isdir(file))
            return;

    if (poptions->size) {
        unsigned long blocks = getblocks(file, poptions);
        printf("%lu ", blocks);
    }

    /*
     * early versions of UNIX printed directories like [this]
     * I'm tempted to do the same, but I'm currently focusing
     * on making it work the same as BSD/GNU ls
     */
    /*
    if (poptions->flags) {
        if (isdir(file))
            putchar('[');
        else if (isexec(file))
            putchar('*');
    }
    */

    if (poptions->color) {
        if (isdir(file))
            putp(poptions->pcolors->blue);
        else if (isexec(file))
            putp(poptions->pcolors->green);
    }

    printf("%s", name);

    if (poptions->color)
        putp(poptions->pcolors->none);

    if (poptions->flags) {
        if (isdir(file))
            /*putchar(']');*/
            putchar('/');
        else if (isexec(file))
            putchar('*');
    }

    putchar('\n');

    free(name);
}

/*
 * casts arguments to the correct type then calls listfile on them
 */
void listfilewalker(void *voidfile, void *poptions)
{
    File *file = (File *)voidfile;

    listfile(file, (Options *)poptions);
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
     *
     * it would be nice to handle reverse order here,
     * but the only way I can think to do it here involves
     * global variables, which seems a bit ugly,
     * so I do it as part of the printing instead
     */
    sortfiles(files, poptions);

    /* 
     * ...then print them
     *
     * poptions->step tells walklist whether to print them in normal order
     * or reverse order (-r flag)
     *
     * listfilewalker just calls listfile on each file
     */
    walklist(files, poptions->step, listfilewalker, poptions);
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
        return blocks / (poptions->blocksize / DEV_BSIZE);
    }
    else {
        return blocks * (DEV_BSIZE / poptions->blocksize);
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
    fprintf(stderr, "Usage: ls2 [-1aCDdFfGkrstU] <file>...\n");
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
