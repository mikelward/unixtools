#ifndef OPTIONS_H
#define OPTIONS_H

#include <sys/types.h>

#include "display.h"
#include "file.h"
#include "logging.h"

#define OPTSTRING "1aBbCcDdEeFfGgIiKkLlMmNnOopqsTtrUux"

/* defaults should be the first element */
enum display { DISPLAY_ONE_PER_LINE, DISPLAY_IN_COLUMNS, DISPLAY_IN_ROWS };
enum escape { ESCAPE_NONE, ESCAPE_QUESTION, ESCAPE_C };
enum flags { FLAGS_NONE, FLAGS_NORMAL, FLAGS_OLD };
enum timetype { TIME_MTIME, TIME_CTIME, TIME_ATIME, TIME_BTIME };
enum sorttype { SORT_BY_NAME, SORT_BY_TIME, SORT_UNSORTED };

/* all the command line options */
/* defaults should usually be 0 */
typedef struct options {
    int all : 1;                    /* 1 = also print hidden files */
    int blocksize;                  /* units for -s option */
    int bytes : 1;                  /* 1 = print file size in bytes */
    int color : 1;                  /* 1 = colorize file and directory names */
    int datetime : 1;               /* 1 = print the file's modification date and time */
    int directory : 1;              /* 1 = print directory name rather than contents */
    int dirsonly : 1;               /* 1 = don't print regular files */
    int dirtotals : 1;              /* 1 = print directory size totals */
    enum display displaymode;       /* one-per-line, columns, rows, etc. */ 
    enum escape escape;             /*     how to handle non-printable characters */
    enum flags flags;               /*     print file "flags" */
    int group : 1;                  /* display the groupname of the file's group */
    int inode : 1;                  /* 1 = print the inode number */
    int linkcount : 1;              /* 1 = print number of hard links */
    int modes : 1;                  /* displays file modes, e.g. -rwxr-xr-x */
    int numeric : 1;                /* 1 = display numeric uid and gid instead of names */
    int owner : 1;                  /* display the username of the file's owner */
    int perms : 1;                  /* display rwx modes for current user */
    int reverse : 1;                /* 0 = forwards, 1 = reverse1 */
    int showlink : 1;               /* 1 = show link -> target in name field (max. 1 link) */
    int showlinks : 1;              /* 1 = show link -> target in name field (resolve all links) */
    int size : 1;                   /* 1 = print file size in blocks */    
    enum sorttype sorttype;         /* how to sort */
    int targetinfo : 1;             /* 1 = field info is based on symlink target */
    enum timetype timetype;         /* which time to show (mtime, ctime, etc.) */

    /* these are more like global state variables than options */
    file_compare_function compare;  /* determines sort order */
    time_t now;                     /* current time - for determining date/time format */
    Colors *colors;                 /* the colors to use */
    short screenwidth;              /* how wide the screen is, 0 if unknown */
    const char *timeformat;         /* custom time format for -T */
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