#include <sys/ioctl.h>
#include <stdlib.h>

#include "options.h"

void setdefaults(Options *options)
{
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
    options->reverse = 0;
    options->showlink = 0;
    options->showlinks = 0;
    options->size = 0;
    options->sorttype = SORT_BY_NAME;
    options->targetinfo = 0;
    options->timetype = TIME_MTIME;

    options->compare = NULL;
    options->now = -1;
    options->pcolors = NULL;
    options->screenwidth = 0;
    options->timeformat = NULL;

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

/**
 * Print the help message.
 */
void usage(void)
{
    fprintf(stderr, "Usage: %s [-%s] [<file>]...\n", myname, OPTSTRING);
}
