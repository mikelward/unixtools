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
    free(options);
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
    options->all = false;
    options->blocksize = 1024;
    options->bytes = false;
    options->color = false;
    options->compatible = true;
    options->datetime = false;
    options->directory = false;
    options->dirsonly = false;
    options->dirtotals = false;
    options->displaymode = DISPLAY_ONE_PER_LINE;
    options->escape = ESCAPE_NONE;
    options->flags = FLAGS_NONE;
    options->followdirlinkargs = DEFAULT; /* see setoptions() for rules */
    options->group = false;
    options->inode = false;
    options->linkcount = false;
    options->longformat = false;
    options->modes = false;
    options->numeric = false;
    options->owner = false;
    options->perms = false;
    options->recursive = false;
    options->reverse = false;
    options->showlink = false;
    options->showlinks = false;
    options->size = false;
    options->sorttype = SORT_BY_NAME;
    options->targetinfo = DEFAULT;
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
            options->all = true;
            break;
        case 'B':
            options->bytes = true;
            break;
        case 'b':
            /* GNU ls uses this for ESCAPE_C, but we use -e = escape */
            options->bytes = true;
            break;
        case 'C':
            options->displaymode = DISPLAY_IN_COLUMNS;
            break;
        case 'c':
            options->timetype = TIME_CTIME;
            /* this interacts with other options see below */
            break;
        case 'D':
            options->dirsonly = true;
            break;
        case 'd':
            options->directory = true;
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
            options->color = true;
            break;
        case 'g':
            /* hopefully saner than legacy ls */
            options->group = true;
            break;
        case 'H':
            options->followdirlinkargs = ON;
            break;
        case 'I':
            options->timeformat = "%Y-%m-%d %H:%M:%S";
            break;
        case 'i':
            options->inode = true;
            break;
        case 'K':
            /* K = "kolor", somewhat mnemonic and unused in GNU ls */
            options->color = true;
            break;
        case 'k':
            options->blocksize = 1024;
            break;
        case 'L':
            options->targetinfo = ON;
            break;
        case 'l':
            options->longformat = true;
            options->modes = true;
            options->linkcount = true;
            options->owner = true;
            options->group = true;
            options->bytes = true;
            options->datetime = true;
            options->showlink = true;
            options->displaymode = DISPLAY_ONE_PER_LINE;
            options->dirtotals = true;
            break;
        case 'M':
            /* might change this to blocksize=1048576 later */
            options->modes = true;
            break;
        case 'm':
            /* conflicts with legacy ls "streams" mode */
            options->modes = true;
            break;
        case 'N':
            options->linkcount = true;
            break;
        case 'n':
            options->numeric = true;
            break;
        case 'O':
            options->flags = FLAGS_OLD;
            break;
        case 'o':
            options->owner = true;
            break;
        case 'P':
            options->targetinfo = OFF;
            break;
        case 'p':
            options->perms = true;
            break;
        case 'q':
            options->escape = ESCAPE_QUESTION;
            break;
        case 'R':
            options->recursive = true;
            break;
        case 'r':
            options->reverse = true;
            break;
        case 'S':
            options->sorttype = SORT_BY_SIZE;
            break;
        case 's':
            options->size = true;
            options->dirtotals = true;
            break;
        case 'T':
            options->datetime = true;
            break;
        case 't':
            options->sorttype = SORT_BY_TIME;
            break;
        case 'U':
            options->sorttype = SORT_UNSORTED;
            break;
        case 'u':
            options->timetype = TIME_ATIME;
            /* this interacts with other options see below */
            break;
        case 'V':
            options->showlinks = true;
            break;
        case 'v':
            options->sorttype = SORT_BY_VERSION;
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

    /* compatibility: -c = -ct, -u = -ut (unless -T or -l) */
    if (options->compatible &&
       !options->datetime &&
        options->timetype != TIME_MTIME) {
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
    case SORT_BY_SIZE:
        /* neither POSIX nor GNU seem to define "size"
           but based on experiments, it seems to be the st_size field */
        options->compare = &comparebysize;
        break;
    case SORT_BY_VERSION:
        options->compare = &comparebyversion;
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

    /* if -H was not given, set it based on other flags...*/
    if (options->followdirlinkargs == DEFAULT) {
        if (options->targetinfo != DEFAULT) {
            /* -L implies -H, -P implies not -H */
            options->followdirlinkargs = options->targetinfo;
        } else if (options->compatible && (options->flags || options->longformat)) {
            /* -F or -l imply not -H */
            options->followdirlinkargs = OFF;
        } else {
            /* if not -L, -P, -F, or -l, -H defaults to on */
            options->followdirlinkargs = ON;
        }
    }

    if (options->targetinfo == DEFAULT) {
        options->targetinfo = OFF;
    }

    if (options->targetinfo == ON) {
      options->showlink = false;
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
