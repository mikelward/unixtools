/*
 * according to POSIX 2008
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/getopt.html>
 * getopt(), optarg, optind, opterr, and optopt are declared by including
 * <unistd.h> rather than <getopt.h>
 */
#define _POSIX_C_SOURCE 200809L /* needed to make getopt() and opt* visible */

#include <sys/ioctl.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
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
    options->sizestyle = SIZE_DEFAULT;
    options->sorttype = SORT_BY_NAME;
    options->targetinfo = DEFAULT;
    options->timestyle = TIME_TRADITIONAL;
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

static struct option longopts[] = {
    /* file selection */
    {"all",                       no_argument,       NULL, 'a'},
    {"directory",                 no_argument,       NULL, 'd'},
    {"dirs-only",                 no_argument,       NULL, 'D'},
    {"recursive",                 no_argument,       NULL, 'R'},

    /* metadata fields */
    {"bytes",                     no_argument,       NULL, 'B'},
    {"group",                     no_argument,       NULL, 'g'},
    {"inode",                     no_argument,       NULL, 'i'},
    {"link-count",                no_argument,       NULL, 'N'},
    {"modes",                     no_argument,       NULL, 'M'},
    {"numeric-uid-gid",           no_argument,       NULL, 'n'},
    {"owner",                     no_argument,       NULL, 'o'},
    {"perms",                     no_argument,       NULL, 'p'},
    {"size",                      no_argument,       NULL, 's'},
    {"show-time",                 no_argument,       NULL, 'T'},

    /* long format */
    {"long",                      no_argument,       NULL, 'l'},

    /* display format */
    {"columns",                   no_argument,       NULL, 'C'},
    {"one-per-line",              no_argument,       NULL, '1'},
    {"rows",                      no_argument,       NULL, 'x'},

    /* sorting */
    {"reverse",                   no_argument,       NULL, 'r'},
    {"unsorted",                  no_argument,       NULL, 'U'},

    /* escaping */
    {"escape",                    no_argument,       NULL, 'e'},
    {"hide-control-chars",        no_argument,       NULL, 'q'},
    {"no-escape",                 no_argument,       NULL, 'E'},

    /* file type indicators */
    {"classify",                  no_argument,       NULL, 'F'},
    {"old-flags",                 no_argument,       NULL, 'O'},

    /* symlink handling */
    {"dereference",               no_argument,       NULL, 'L'},
    {"dereference-command-line",  no_argument,       NULL, 'H'},
    {"no-dereference",            no_argument,       NULL, 'P'},
    {"show-links",                no_argument,       NULL, 'V'},

    /* size/time formatting */
    {"color",                     no_argument,       NULL, 'G'},
    {"human-readable",            no_argument,       NULL, 'h'},
    {"iso",                       no_argument,       NULL, 'I'},
    {"kibibytes",                 no_argument,       NULL, 'k'},

    /* time type shortcuts */
    {"atime",                     no_argument,       NULL, 0  },
    {"btime",                     no_argument,       NULL, 0  },
    {"ctime",                     no_argument,       NULL, 0  },
    {"mtime",                     no_argument,       NULL, 0  },

    /* GNU-compatible long options with arguments */
    {"format",                    required_argument, NULL, 0  },
    {"sort",                      required_argument, NULL, 0  },
    {"time",                      required_argument, NULL, 0  },
    {"time-style",                required_argument, NULL, 0  },

    {NULL, 0, NULL, 0},
};

static int longindex = 0;

int setoptions(Options *options, int argc, char **argv)
{
    opterr = 0;     /* we will print our own error messages */
    int option;
    while ((longindex = 0, option = getopt_long(argc, argv, ":" OPTSTRING, longopts, &longindex)) != -1) {
        switch(option) {
        case 0:
            if (strcmp(longopts[longindex].name, "atime") == 0) {
                options->timetype = TIME_ATIME;
            } else if (strcmp(longopts[longindex].name, "btime") == 0) {
                options->timetype = TIME_BTIME;
            } else if (strcmp(longopts[longindex].name, "ctime") == 0) {
                options->timetype = TIME_CTIME;
            } else if (strcmp(longopts[longindex].name, "mtime") == 0) {
                options->timetype = TIME_MTIME;
            } else if (strcmp(longopts[longindex].name, "time-style") == 0) {
                options->datetime = true;
                if (strcmp(optarg, "traditional") == 0) {
                    options->timestyle = TIME_TRADITIONAL;
                } else if (strcmp(optarg, "relative") == 0) {
                    options->timestyle = TIME_RELATIVE;
                } else {
                    error("Unsupported time-style '%s'\n", optarg);
                    exit(2);
                }
            } else if (strcmp(longopts[longindex].name, "sort") == 0) {
                if (strcmp(optarg, "none") == 0) {
                    options->sorttype = SORT_UNSORTED;
                } else if (strcmp(optarg, "name") == 0) {
                    options->sorttype = SORT_BY_NAME;
                } else if (strcmp(optarg, "size") == 0) {
                    options->sorttype = SORT_BY_SIZE;
                } else if (strcmp(optarg, "time") == 0) {
                    options->sorttype = SORT_BY_TIME;
                } else if (strcmp(optarg, "version") == 0) {
                    options->sorttype = SORT_BY_VERSION;
                } else {
                    error("Unsupported sort type '%s'\n", optarg);
                    exit(2);
                }
            } else if (strcmp(longopts[longindex].name, "time") == 0) {
                if (strcmp(optarg, "mtime") == 0 || strcmp(optarg, "modification") == 0) {
                    options->timetype = TIME_MTIME;
                } else if (strcmp(optarg, "atime") == 0 || strcmp(optarg, "access") == 0 || strcmp(optarg, "use") == 0) {
                    options->timetype = TIME_ATIME;
                } else if (strcmp(optarg, "ctime") == 0 || strcmp(optarg, "status") == 0) {
                    options->timetype = TIME_CTIME;
                } else if (strcmp(optarg, "btime") == 0 || strcmp(optarg, "birth") == 0 || strcmp(optarg, "creation") == 0) {
                    options->timetype = TIME_BTIME;
                } else {
                    error("Unsupported time type '%s'\n", optarg);
                    exit(2);
                }
            } else if (strcmp(longopts[longindex].name, "format") == 0) {
                if (strcmp(optarg, "long") == 0 || strcmp(optarg, "verbose") == 0) {
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
                } else if (strcmp(optarg, "single-column") == 0) {
                    options->displaymode = DISPLAY_ONE_PER_LINE;
                } else if (strcmp(optarg, "vertical") == 0 || strcmp(optarg, "columns") == 0) {
                    options->displaymode = DISPLAY_IN_COLUMNS;
                } else if (strcmp(optarg, "across") == 0 || strcmp(optarg, "horizontal") == 0) {
                    options->displaymode = DISPLAY_IN_ROWS;
                } else {
                    error("Unsupported format '%s'\n", optarg);
                    exit(2);
                }
            }
            break;
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
        case 'h':
            options->sizestyle = SIZE_HUMAN;
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
        case TIME_BTIME:
            options->compare = &comparebybtime;
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

    if (options->datetime) {
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
    fprintf(stderr,
        "Usage: %s [-%s] [OPTION]... [<file>]...\n"
        "\n"
        "File selection:\n"
        "  -a, --all                  show hidden files\n"
        "  -D, --dirs-only            show only directories\n"
        "  -d, --directory            list directory names, not contents\n"
        "  -R, --recursive            list subdirectories recursively\n"
        "\n"
        "Metadata fields:\n"
        "  -B, -b, --bytes            show file size in bytes\n"
        "  -g, --group                show group\n"
        "  -i, --inode                show inode number\n"
        "  -M, -m, --modes            show file modes\n"
        "  -N, --link-count           show hard link count\n"
        "  -n, --numeric-uid-gid      show numeric uid/gid\n"
        "  -o, --owner                show owner\n"
        "  -p, --perms                show current user's permissions\n"
        "  -s, --size                 show size in blocks\n"
        "  -T, --show-time            show date/time\n"
        "\n"
        "Long format:\n"
        "  -l, --long                 long format (equivalent to -MNogBT1)\n"
        "\n"
        "Display format:\n"
        "  -C, --columns              multi-column output (sorted down)\n"
        "  -x, --rows                 multi-column output (sorted across)\n"
        "  -1, --one-per-line         one entry per line\n"
        "\n"
        "Sorting:\n"
        "  -r, --reverse              reverse sort order\n"
        "  -S                         sort by size\n"
        "  -t                         sort by time\n"
        "  -v                         sort by version\n"
        "  -U, -f, --unsorted         do not sort\n"
        "\n"
        "Escaping:\n"
        "  -e, --escape               C-style escape sequences\n"
        "  -E, --no-escape            no escaping\n"
        "  -q, --hide-control-chars   replace control chars with '?'\n"
        "\n"
        "File type indicators:\n"
        "  -F, --classify             append file type indicators\n"
        "  -O, --old-flags            BSD-style file type indicators\n"
        "\n"
        "Symlink handling:\n"
        "  -H, --dereference-command-line  follow symlinks to dirs in args\n"
        "  -L, --dereference          follow all symlinks\n"
        "  -P, --no-dereference       never follow symlinks\n"
        "  -V, --show-links           show full symlink chain\n"
        "\n"
        "Formatting:\n"
        "  -G, -K, --color            colorize output\n"
        "  -h, --human-readable       human-readable sizes\n"
        "  -I, --iso                  ISO 8601 date format\n"
        "  -k, --kibibytes            use 1024-byte blocks\n"
        "  -c                         use ctime\n"
        "  -u                         use atime\n"
        "      --format=FORMAT        output format: long, single-column,\n"
        "                               vertical, across\n"
        "      --sort=WORD            sort by: name, size, time, version, none\n"
        "      --time=WORD            time type: mtime, atime, ctime, btime\n"
        "      --time-style=STYLE     time format: traditional, relative\n",
        myname, OPTSTRING);
}
