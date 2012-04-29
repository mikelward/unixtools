/*
 * according to POSIX 2008
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/getopt.html>
 * getopt(), optarg, optind, opterr, and optopt are declared by including
 * <unistd.h> rather than <getopt.h>
 */
#define _POSIX_C_SOURCE 200809L /* needed to make getopt() and opt* visible */

#include <sys/ioctl.h>
#include <stdlib.h>
#include <time.h>

#include "display.h"
#include "logging.h"
#include "map.h"
#include "options.h"

void freeoptions(Options *options)
{
    if (!options) return;
    freemap(options->usernames);
    freemap(options->groupnames);
    freecolors(options->colors);
}

Options *newoptions(void)
{
    Options *options = malloc(sizeof(*options));
    if (!options) {
        errorf("Out of memory?\n");
        return NULL;
    }
    setdefaults(options);
    return options;
}

void setdefaults(Options *options)
{
    /* TODO use memset? */
    options->all = 0;
    options->blocksize = 1024;
    options->bytes = 0;
    options->color = 0;
    options->datetime = 0;
    options->directory = 0;
    options->dirsonly = 0;
    options->dirtotals = 0;
    options->displaymode = DISPLAY_ONE_PER_LINE;
    options->escape = ESCAPE_NONE;
    options->flags = FLAGS_NONE;
    options->group = 0;
    options->inode = 0;
    options->linkcount = 0;
    options->modes = 0;
    options->numeric = 0;
    options->owner = 0;
    options->perms = 0;
    options->recursive = 0;
    options->reverse = 0;
    options->showlink = 0;
    options->showlinks = 0;
    options->size = 0;
    options->sorttype = SORT_BY_NAME;
    options->targetinfo = 0;
    options->timetype = TIME_MTIME;

    options->compare = NULL;
    options->groupnames = NULL;
    options->now = -1;
    options->colors = NULL;
    options->screenwidth = 0;
    options->timeformat = NULL;
    options->usernames = NULL;

    /* use BLOCKSIZE as default blocksize if set */
    char *blocksizeenv = getenv("BLOCKSIZE");
    if (blocksizeenv != NULL) {
        int blocksize = atoi(blocksizeenv);
        if (blocksize != 0) {
            options->blocksize = blocksize;
        }
    }

    /* if output is a terminal, turn on -C and -q */
    if (isatty(STDOUT_FILENO)) {
        struct winsize ws;
        if ((ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)) == 0) {
            int width = ws.ws_col;
            if (width != 0) {
                options->screenwidth = width;
                options->displaymode = DISPLAY_IN_COLUMNS;
            }
        }
        options->escape = ESCAPE_QUESTION;
    }
    /* default width is 80 so we can do columns or rows
     * if the user explicitly specified the -C or -x option
     * (as per BSD and GNU) */
    if (options->screenwidth == 0)
        options->screenwidth = 80;
}

int setoptions(Options *options, int argc, char **argv)
{
    opterr = 0;     /* we will print our own error messages */
    int option;
    while ((option = getopt(argc, argv, ":" OPTSTRING)) != -1) {
        switch(option) {
        case '1':
            options->displaymode = DISPLAY_ONE_PER_LINE;
            break;
        case 'A':
            /* maybe print ACLs?  or -a without "." and ".."? */
            break;
        case 'a':
            options->all = 1;
            break;
        case 'B':
            options->bytes = 1;
            break;
        case 'b':
            /* GNU ls uses this for ESCAPE_C, but we use -e = escape */
            options->bytes = 1;
            break;
        case 'C':
            options->displaymode = DISPLAY_IN_COLUMNS;
            break;
        case 'c':
            options->timetype = TIME_CTIME;
            /* this interacts with other options-> see below */
            break;
        case 'D':
            options->dirsonly = 1;
            break;
        case 'd':
            options->directory = 1;
            break;
        case 'E':
            options->escape = ESCAPE_NONE;
            break;
        case 'e':
            options->escape = ESCAPE_C;
            break;
        case 'F':
            options->flags = FLAGS_NORMAL;
            break;
        case 'f':
            options->compare = NULL;
            break;
        case 'G':
            /* for compatibility with FreeBSD */
            options->color = 1;
            break;
        case 'g':
            /* hopefully saner than legacy ls */
            options->group = 1;
            break;
        case 'K':
            /* K = "kolor", somewhat mnemonic and unused in GNU ls */
            options->color = 1;
            break;
        case 'k':
            options->blocksize = 1024;
            break;
        case 'I':
            options->timeformat = "%Y-%m-%d %H:%M:%S";
            break;
        case 'i':
            options->inode = 1;
            break;
        case 'L':
            options->showlinks = 1;
            options->targetinfo = 1;
            break;
        case 'l':
            options->modes = 1;
            options->linkcount = 1;
            options->owner = 1;
            options->group = 1;
            options->bytes = 1;
            options->datetime = 1;
            options->showlink = 1;
            options->displaymode = DISPLAY_ONE_PER_LINE;
            options->dirtotals = 1;
            break;
        case 'M':
            /* might change this to blocksize=1048576 later */
            options->modes = 1;
            break;
        case 'm':
            /* conflicts with legacy ls "streams" mode */
            options->modes = 1;
            break;
        case 'N':
            options->linkcount = 1;
            break;
        case 'n':
            options->numeric = 1;
            break;
        case 'O':
            options->flags = FLAGS_OLD;
            break;
        case 'o':
            options->owner = 1;
            break;
        case 'P':
            /* reserved for physical mode (don't follow symlinks) */
            break;
        case 'p':
            options->perms = 1;
            break;
        case 'q':
            options->escape = ESCAPE_QUESTION;
            break;
        case 'R':
            options->recursive = 1;
            break;
        case 'r':
            options->reverse = 1;
            break;
        case 'S':
            /* possibly reserved for sort by size option */
            break;
        case 's':
            options->size = 1;
            options->dirtotals = 1;
            break;
        case 'T':
            options->datetime = 1;
            break;
        case 't':
            options->sorttype = SORT_BY_TIME;
            break;
        case 'U':
            options->sorttype = SORT_UNSORTED;
            break;
        case 'u':
            options->timetype = TIME_ATIME;
            /* this interacts with other options-> see below */
            break;
        case 'x':
            options->displaymode = DISPLAY_IN_ROWS;
            break;
        case ':':
            error("Missing argument to -%c\n", optopt);
            usage();
            return -1;
        case '?':
            error("Unknown option -%c\n", optopt);
            usage();
            return -1;
        default:
            usage();
            return -1;
        }
    }

    /* -c = -ct, -u = -ut (unless -T or -l) */
    if (!options->datetime && options->timetype != TIME_MTIME) {
        options->sorttype = SORT_BY_TIME;
    }

    switch (options->sorttype) {
    case SORT_BY_NAME:
        options->compare = &comparebyname;
        break;
    case SORT_BY_TIME:
        /* XXX make this cleaner */
        switch (options->timetype) {
        case TIME_MTIME:
            options->compare = &comparebymtime;
            break;
        case TIME_ATIME:
            options->compare = &comparebyatime;
            break;
        case TIME_CTIME:
            options->compare = &comparebyctime;
            break;
        default:
            errorf("Unknown time type\n");
            options->compare = &comparebyname;
            break;
        }
        break;
    case SORT_UNSORTED:
        options->compare = NULL;
        /* don't reverse output if it's unsorted
         * (as per BSD and GNU) */
        if (options->compare == NULL) {
            options->reverse = 0;
        }
        break;
    }

    if (options->color) {
        Colors *colors = malloc(sizeof(*colors));
        if (!colors) {
            errorf("Out of memory?\n");
            goto error;
        }

        options->color = setupcolors(colors);
        options->colors = colors;
    }

    if (options->datetime && options->timeformat == NULL) {
        options->now = time(NULL);
        if (options->now == -1) {
            errorf("Cannot determine current time\n");
            /* non-fatal */
        }
    }

    if (options->group && !options->numeric) {
        options->groupnames = newmap();
        if (!options->groupnames) {
            errorf("Out of memory?\n");
            goto error;
        }
    }
    if (options->owner && !options->numeric) {
        options->usernames = newmap();
        if (!options->usernames) {
            errorf("Out of memory?\n");
            goto error;
        }
    }

    return optind;

error:
    freeoptions(options);
    return -1;
}

/**
 * Print the help message.
 */
void usage(void)
{
    fprintf(stderr, "Usage: %s [-%s] [<file>]...\n", myname, OPTSTRING);
}
