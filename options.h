#ifndef OPTIONS_H
#define OPTIONS_H

#include <sys/types.h>
#include <stdbool.h>

#include "buf.h"
#include "display.h"
#include "file.h"
#include "logging.h"
#include "map.h"

#define OPTSTRING "1aBbCcDdEeFfGgHIiKkLlMmNnOoPpqRrSsTtUuvx"

/* defaults should be the first element */
enum display { DISPLAY_ONE_PER_LINE, DISPLAY_IN_COLUMNS, DISPLAY_IN_ROWS };
enum flags { FLAGS_NONE, FLAGS_NORMAL, FLAGS_OLD };
enum timetype { TIME_MTIME, TIME_CTIME, TIME_ATIME, TIME_BTIME };
enum sorttype { SORT_BY_NAME, SORT_BY_TIME, SORT_BY_SIZE, SORT_UNSORTED, SORT_BY_VERSION };
enum tri { DEFAULT = -1, OFF = 0, ON = 1 };

/* all the command line options */
/* defaults should usually be 0 */
typedef struct options {
    bool all : 1;                   /* true = also show hidden files */
    unsigned blocksize;             /* units for -s option */
    bool bytes : 1;                 /* true = show file size in bytes */
    bool compatible : 1;            /* true = do complicated stuff for compatibility */
    bool color : 1;                 /* true = colorize file and directory names */
    bool datetime : 1;              /* true = show the file's modification date and time */
    bool directory : 1;             /* true = show directory name rather than contents */
    bool dirsonly : 1;              /* true = only list directories, not regular files */
    bool dirtotals : 1;             /* true = show directory size totals */
    enum display displaymode;       /* one-per-line, columns, rows, etc. */ 
    enum escape escape;             /*     how to handle non-printable characters */
    enum flags flags;               /*     show file "flags" */
    enum tri followdirlinkargs : 2; /* ON = dereference links to dirs in args */
    bool group : 1;                 /* true = show the file's group */
    bool inode : 1;                 /* true = show the inode number */
    bool linkcount : 1;             /* true = show number of hard links */
    bool longformat : 1;            /* true = long format */
    bool modes : 1;                 /* true = show the file's modes, e.g. -rwxr-xr-x */
    bool numeric : 1;               /* true = show uid and gid instead of username and groupname */
    bool owner : 1;                 /* true = show the file's owner */
    bool perms : 1;                 /* true = show permissions for the current user, e.g. rwx */
    bool recursive : 1;             /* true = after listing a directory, list its subdirectories recursively */
    bool reverse : 1;               /* true = sort oldest to newest (or smallest to largest with -S option) */
    bool showlink : 1;              /* true = show link -> target in name field (max. 1 link) */
    bool showlinks : 1;             /* true = show link -> target in name field (resolve all links) */
    bool size : 1;                  /* true = show file size in blocks */
    enum sorttype sorttype;         /* how to sort */
    enum tri targetinfo : 2;        /* ON = field info is based on symlink target */
    enum timetype timetype;         /* which time to show (mtime, ctime, etc.) */

    /* these are more like global state variables than options */
    file_compare_function compare;  /* determines sort order */
    Map *groupnames;                /* cache of gid -> groupname for -g */
    time_t now;                     /* current time - for determining date/time format */
    Colors *colors;                 /* the colors to use */
    short screenwidth;              /* how wide the screen is, 0 if unknown */
    const char *timeformat;         /* custom time format for -T */
    Map *usernames;                 /* cache of uid -> username for -o */
} Options;

/**
 * Frees options and any memory it allocated.
 */
void freeoptions(Options *options);

/**
 * Create a new options object, initialized to defaults.
 *
 * @todo: Move terminal handling into its own thing?
 */
Options *newoptions(void);

/**
 * Initialize options.
 */
void setdefaults(Options *options);

/**
 * Set options based on supplied command line arguments.
 *
 * Returns number of arguments processed on success, or -1 on error.
 */
int setoptions(Options *options, int argc, char **argv);

/**
 * Print a brief description of how to run the program to stderr.
 */
void usage(void);

#endif
