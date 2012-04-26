/*
 * cleaner reimplementation of ls
 *
 * TODO
 * - handling of symlink arguments (and -H and -L flags?)
 * - handling of symlink arguments (-F, link ending in slash, etc.)
 * - -S flag
 * - correctly calculate column width of extended ("wide") characters
 * - remove remaining statically-sized buffers (search for 1024)
 * - fix user and group lookups, cache them in a hash
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
#include <ctype.h>
#include <curses.h>
#include <dirent.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <term.h>
#include <time.h>
#include <unistd.h>

#include "buf.h"
#include "display.h"
#include "field.h"
#include "file.h"
#include "list.h"
#include "logging.h"
#include "options.h"

typedef List FileList;              /* list of files */
typedef List FieldList;             /* list of fields for a single file */
typedef List FileFieldList;         /* list of fields for each file */

const int columnmargin = 1;

int  listfile(File *file, Options *poptions);
void listfilewithnewline(File *file, Options *poptions);
void listfiles(List *files, Options *poptions);
void listdir(File *dir, Options *poptions);
void printnametobuf(const char *name, Options *poptions, Buf *buf);
int  printsize(File *file, Options *poptions);
int  setupcolors(Colors *pcolors);
void sortfiles(List *files, Options *poptions);
void usage(void);
int  want(File *file, Options *poptions);

#define OPTSTRING "1aBbCcDdEeFfGgIiKkLlMmNnOopqsTtrUux"

int main(int argc, char **argv)
{
    myname = "l";
    /* so that file names are sorted according to the user's locale */
    setlocale(LC_ALL, "");

    /* TODO use memset? */
    Options options;
    options.all = 0;
    options.blocksize = 1024;
    options.bytes = 0;
    options.datetime = 0;
    options.directory = 0;
    options.dirsonly = 0;
    options.dirtotals = 0;
    options.displaymode = DISPLAY_ONE_PER_LINE;
    options.escape = ESCAPE_NONE;
    options.color = 0;
    options.flags = FLAGS_NONE;
    options.inode = 0;
    options.linkcount = 0;
    options.showlink = 0;
    options.showlinks = 0;
    options.targetinfo = 0;
    options.modes = 0;
    options.numeric = 0;
    options.now = -1;
    options.owner = 0;
    options.group = 0;
    options.perms = 0;
    options.size = 0;
    options.reverse = 0;
    options.screenwidth = 0;
    options.compare = NULL;
    options.pcolors = NULL;
    options.timeformat = NULL;
    options.timetype = TIME_MTIME;
    options.sorttype = SORT_BY_NAME;

    /* use BLOCKSIZE as default blocksize if set */
    char *blocksizeenv = getenv("BLOCKSIZE");
    if (blocksizeenv != NULL) {
        int blocksize = atoi(blocksizeenv);
        if (blocksize != 0) {
            options.blocksize = blocksize;
        }
    }

    /* if output is a terminal, turn on -C and -q */
    if (isatty(STDOUT_FILENO)) {
        struct winsize ws;
        if ((ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)) == 0) {
            int width = ws.ws_col;
            if (width != 0) {
                options.screenwidth = width;
                options.displaymode = DISPLAY_IN_COLUMNS;
            }
        }
        options.escape = ESCAPE_QUESTION;
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
        case 'A':
            /* maybe print ACLs?  or -a without "." and ".."? */
            break;
        case 'a':
            options.all = 1;
            break;
        case 'B':
            options.bytes = 1;
            break;
        case 'b':
            /* GNU ls uses this for ESCAPE_C, but we use -e = escape */
            options.bytes = 1;
            break;
        case 'C':
            options.displaymode = DISPLAY_IN_COLUMNS;
            break;
        case 'c':
            options.timetype = TIME_CTIME;
            /* this interacts with other options, see below */
            break;
        case 'D':
            options.dirsonly = 1;
            break;
        case 'd':
            options.directory = 1;
            break;
        case 'E':
            options.escape = ESCAPE_NONE;
            break;
        case 'e':
            options.escape = ESCAPE_C;
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
            /* hopefully saner than legacy ls */
            options.group = 1;
            break;
        case 'K':
            /* K = "kolor", somewhat mnemonic and unused in GNU ls */
            options.color = 1;
            break;
        case 'k':
            options.blocksize = 1024;
            break;
        case 'I':
            options.timeformat = "%Y-%m-%d %H:%M:%S";
            break;
        case 'i':
            options.inode = 1;
            break;
        case 'L':
            options.showlinks = 1;
            options.targetinfo = 1;
            break;
        case 'l':
            options.modes = 1;
            options.linkcount = 1;
            options.owner = 1;
            options.group = 1;
            options.bytes = 1;
            options.datetime = 1;
            options.showlink = 1;
            options.displaymode = DISPLAY_ONE_PER_LINE;
            options.dirtotals = 1;
            break;
        case 'M':
            /* might change this to blocksize=1048576 later */
            options.modes = 1;
            break;
        case 'm':
            /* conflicts with legacy ls "streams" mode */
            options.modes = 1;
            break;
        case 'N':
            options.linkcount = 1;
            break;
        case 'n':
            options.numeric = 1;
            break;
        case 'O':
            options.flags = FLAGS_OLD;
            break;
        case 'o':
            options.owner = 1;
            break;
        case 'P':
            /* reserved for physical mode (don't follow symlinks) */
            break;
        case 'p':
            options.perms = 1;
            break;
        case 'q':
            options.escape = ESCAPE_QUESTION;
            break;
        case 'r':
            options.reverse = 1;
            break;
        case 'S':
            /* possibly reserved for sort by size option */
            break;
        case 's':
            options.size = 1;
            options.dirtotals = 1;
            break;
        case 'T':
            options.datetime = 1;
            break;
        case 't':
            options.sorttype = SORT_BY_TIME;
            break;
        case 'U':
            options.sorttype = SORT_UNSORTED;
            break;
        case 'u':
            options.timetype = TIME_ATIME;
            /* this interacts with other options, see below */
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

    /* -c = -ct, -u = -ut (unless -T or -l) */
    if (!options.datetime && options.timetype != TIME_MTIME) {
        options.sorttype = SORT_BY_TIME;
    }

    switch (options.sorttype) {
    case SORT_BY_NAME:
        options.compare = &comparebyname;
        break;
    case SORT_BY_TIME:
        /* XXX make this cleaner */
        switch (options.timetype) {
        case TIME_MTIME:
            options.compare = &comparebymtime;
            break;
        case TIME_ATIME:
            options.compare = &comparebyatime;
            break;
        case TIME_CTIME:
            options.compare = &comparebyctime;
            break;
        default:
            error("Unknown time type\n");
            options.compare = &comparebyname;
            break;
        }
        break;
    case SORT_UNSORTED:
        options.compare = NULL;
        /* don't reverse output if it's unsorted
         * (as per BSD and GNU) */
        if (options.compare == NULL) {
            options.reverse = 0;
        }
        break;
    }

    Colors colors;
    if (options.color) {
        options.color = setupcolors(&colors);
        options.pcolors = &colors;
    }

    if (options.datetime && options.timeformat == NULL) {
        options.now = time(NULL);
        if (options.now == -1) {
            error("Cannot determine current time\n");
        }
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
        File *file = newfile(".", argv[i]);
        if (file == NULL) {
            error("file is NULL\n");
            exit(1);
        }
        if (isstat(file)) {
            if (!options.directory && isdir(file)) {
                append(file, dirs);
            } else {
                append(file, files);
            }
        }
    }

    int nfiles = length(files);
    listfiles(files, &options);
    freelist(files, (free_func)freefile);

    /*
     * XXX make this use walklist
     */
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

    freelist(dirs, (free_func)freefile);
}

void getnamefieldhelper(File *file, Options *poptions, Buf *buf, int showpath)
{
    assert(file != NULL);
    assert(poptions != NULL);
    assert(buf != NULL);

    /*
     * print a character *before* the file showing its type (-O)
     *
     * early versions of BSD printed directories like [this]
     */
    switch (poptions->flags) {
    case FLAGS_NORMAL:
        break;
    case FLAGS_OLD:
        if (!isstat(file))
            bufappend(buf, "?", 1, 1);
        else if (isdir(file))
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
        if (!isstat(file)) {
            bufappend(buf, poptions->pcolors->red, strlen(poptions->pcolors->red), 0);
            colorused = 1;
        } else if (isdir(file)) {
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

    const char *name = getname(file);
    printnametobuf(name, poptions, buf);
    /* don't free name */

    /* reset the color back to normal (-G and -K) */
    if (colorused) {
        bufappend(buf, poptions->pcolors->none, strlen(poptions->pcolors->none), 0);
    }

    /* print a character after the file showing its type (-F and -O) */
    switch (poptions->flags) {
    case FLAGS_NORMAL:
        if (!isstat(file))
            bufappend(buf, "?", 1, 1);
        else if (isdir(file))
            bufappend(buf, "/", 1, 1);
        else if (islink(file))
            bufappend(buf, "@", 1, 1);
        else if (isfifo(file))
            bufappend(buf, "|", 1, 1);
        else if (issock(file))
            bufappend(buf, "=", 1, 1);
        else if (isexec(file))
            bufappend(buf, "*", 1, 1);
        else
            bufappend(buf, " ", 1, 1);
        break;
    case FLAGS_OLD:
        if (!isstat(file))
            bufappend(buf, "?", 1, 1);
        else if (isdir(file))
            bufappend(buf, "]", 1, 1);
        else if (islink(file))
            bufappend(buf, "@", 1, 1);
        else if (isexec(file))
            bufappend(buf, "*", 1, 1);
        else
            bufappend(buf, " ", 1, 1);
        break;
    case FLAGS_NONE:
        break;
    }
}

Field *getnamefield(File *file, Options *poptions)
{
    if (file == NULL) {
        errorf("file is NULL\n");
        return NULL;
    }
    if (poptions == NULL) {
        errorf("options is NULL\n");
        return NULL;
    }

    Buf *buf = newbuf();
    if (buf == NULL) {
        errorf("buf is NULL\n");
        return NULL;
    }

    /* print the file itself... */
    getnamefieldhelper(file, poptions, buf, 0);

    if (poptions->showlinks) {
        /* resolve and print symlink targets recursively */
        while (isstat(file) && islink(file)) {
            file = gettarget(file);
            bufappend(buf, " -> ", 4, 4);
            getnamefieldhelper(file, poptions, buf, 1);
        }
    } else if (poptions->showlink) {
        /* print only the first link target without stat'ing the target */
        if (isstat(file) && islink(file)) {
            file = gettarget(file);
            bufappend(buf, " -> ", 4, 4);
            getnamefieldhelper(file, poptions, buf, 1);
        }
    }

    enum align align = ALIGN_NONE;
    if (poptions->displaymode != DISPLAY_ONE_PER_LINE) {
        align = ALIGN_LEFT;
    }

    Field *field = newfield(bufstring(buf), align, bufscreenpos(buf));
    if (field == NULL) {
        errorf("field is NULL\n");
        return NULL;
    }
    freebuf(buf);
    return field;
}


/**
 * Returns a list of Fields for the given file.
 *
 * Which fields are returned is controlled by poptions.
 */
FieldList *getfields(File *file, Options *poptions)
{
    if (file == NULL) {
        errorf("file is NULL\n");
        return NULL;
    }

    char snprintfbuf[1024];
    List *fieldlist = newlist();
    if (fieldlist == NULL) {
        errorf("fieldlist is NULL\n");
        return NULL;
    }

    /*
     * with -L, display information about the link target file by setting file = target
     * the originally named file is saved as "link" for displaying the name
     */
    File *link = file;
    if (poptions->targetinfo) {
        File *target = NULL;
        while (isstat(file) && islink(file)) {
            target = gettarget(file);
            if (target == NULL) {
                errorf("target is NULL\n");
                walklist(fieldlist, free);
                free(fieldlist);
                return NULL;
            }
            file = target;
        }
    }

    if (poptions->size) {
        int width;
        if (isstat(file)) {
            unsigned long blocks = getblocks(file, poptions->blocksize);
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%lu", blocks);
        } else {
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%s", "?");
        }
        Field *field = newfield(snprintfbuf, ALIGN_RIGHT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (poptions->inode) {
        int width;
        if (isstat(file)) {
            ino_t inode = getinode(file);
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%lu", inode);
        } else {
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%s", "?");
        }
        Field *field = newfield(snprintfbuf, ALIGN_RIGHT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (poptions->modes) {
        int width;
        if (isstat(file)) {
            char *modes = getmodes(file);
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%s", modes);
        } else {
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%s", "???????????");
        }
        Field *field = newfield(snprintfbuf, ALIGN_LEFT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (poptions->linkcount) {
        int width;
        if (isstat(file)) {
            nlink_t nlinks = getlinkcount(file);
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%lu", (unsigned long)nlinks);
        } else {
             width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%s", "?");
        }
        Field *field = newfield(snprintfbuf, ALIGN_RIGHT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (poptions->owner) {
        int width;
        if (isstat(file)) {
            if (poptions->numeric) {
                // TODO error handling
                uid_t uid = getownernum(file);
                width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%lu", (unsigned long)uid);
            } else {
                char *owner = getownername(file);
                width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%s", owner);
            }
        } else {
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "?");
        }
        Field *field = newfield(snprintfbuf, ALIGN_LEFT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (poptions->group) {
        int width;
        if (isstat(file)) {
            if (poptions->numeric) {
                // TODO error handling
                gid_t gid = getgroupnum(file);
                width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%lu", (unsigned long)gid);
            } else {
                char *group = getgroupname(file);
                width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%s", group);
            }
        } else {
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "?");
        }
        Field *field = newfield(snprintfbuf, ALIGN_LEFT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (poptions->perms) {
        char *perms = getperms(file);
        if (perms == NULL) {
            errorf("perms is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        int width = strlen(perms);
        Field *field = newfield(perms, ALIGN_RIGHT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
        free(perms);
    }

    if (poptions->bytes) {
        int width;
        if (isstat(file)) {
            long bytes = getsize(file);
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "%ld", bytes);
        } else {
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "?");
        }
        Field *field = newfield(snprintfbuf, ALIGN_RIGHT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    if (poptions->datetime) {
        int width;
        if (isstat(file)) {
            time_t timestamp;
            switch (poptions->timetype) {
            case TIME_ATIME:
                timestamp = getatime(file);
                break;
            case TIME_CTIME:
                timestamp = getctime(file);
                break;
            /*
            case TIME_BTIME:
                timestamp = getbtime(file);
                break;
            */
            default:
                error("Unknown time attribute\n");
                /* fall through */
            case TIME_MTIME:
                timestamp = getmtime(file);
                break;
            }
            struct tm *timestruct = localtime(&timestamp);
            if (timestruct == NULL) {
                errorf("timestruct is NULL\n");
                walklist(fieldlist, free);
                free(fieldlist);
                return NULL;
            }
            if (poptions->timeformat != NULL) {
                width = strftime(snprintfbuf, sizeof(snprintfbuf), poptions->timeformat, timestruct);
            } else {
                /* month day hour and minute if file was modified in the last 6 months,
                   month day year otherwise */
                if (poptions->now != -1 && timestamp <= poptions->now && timestamp > poptions->now - 180*86400) {
                    width = strftime(snprintfbuf, sizeof(snprintfbuf), "%b %e %H:%M", timestruct);
                } else {
                    width = strftime(snprintfbuf, sizeof(snprintfbuf), "%b %e  %Y", timestruct);
                }
            }
        } else {
            width = snprintf(snprintfbuf, sizeof(snprintfbuf), "?");
        }
        Field *field = newfield(snprintfbuf, ALIGN_RIGHT, width);
        if (field == NULL) {
            errorf("field is NULL\n");
            walklist(fieldlist, free);
            free(fieldlist);
            return NULL;
        }
        append(field, fieldlist);
    }

    /*
     * print the file name with flags and colors if requested
     * flags, color start, filename, color end, and flags are made into a single field
     * since we don't want spaces in between
     * if -L is given and the file is a symlink, the returned fields include the name
     * of the link's target (multiple times if there are multiple links)
     * i.e. link -> file
     * or even link -> link -> link -> file
     */
    Field *field = getnamefield(link, poptions);
    append(field, fieldlist);
    
    return (FieldList *)fieldlist;
}

void printwithnewline(void *string)
{
    puts((char *)string);
}

StringList *makefilestrings(FileFieldList *filefields, int *fieldwidths)
{
    if (filefields == NULL) {
        errorf("filefields is NULL\n");
        return NULL;
    }
    if (fieldwidths == NULL) {
        errorf("fieldwidths is NULL\n");
        return NULL;
    }

    char snprintfbuf[1024];
    StringList *filestrings = newlist();
    if (filestrings == NULL) {
        errorf("filestrings is NULL\n");
        return NULL;
    }

    int nfiles = length(filefields);
    for (int i = 0; i < nfiles; i++) {
        Buf *buf = newbuf();
        if (buf == NULL) {
            errorf("buf is NULL\n");
            freelist(filestrings, (free_func)free);
            return NULL;
        }
        /* XXX could be simpler? */
        List *fields = getitem(filefields, i);
        int nfields = length(fields);
        for (int j = 0; j < nfields; j++) {
            Field *field = getitem(fields, j);
            if (field == NULL) {
                errorf("field is NULL\n");
                freelist(filestrings, (free_func)free);
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
        append(bufstring(buf), filestrings);
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
        errorf("filefields is NULL\n");
        return NULL;
    }
    int nfiles = length(filefields);
    if (nfiles == 0) {
        return NULL;
    }

    FieldList *firstfilefields = getitem(filefields, 0);
    if (firstfilefields == NULL) {
        errorf("firstfilefields is NULL\n");
        return NULL;
    }
    int nfields = length(firstfilefields);
    int *maxfieldwidths = calloc(nfields+1, sizeof(*maxfieldwidths));
    for (int i = 0; i < nfiles; i++) {
        FieldList *fields = getitem(filefields, i);
        if (fields == NULL) {
            errorf("fields is NULL\n");
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
        errorf("files is NULL\n");
        return;
    } else if (poptions == NULL) {
        errorf("poptions is NULL\n");
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
    /* we don't own files, so don't free it here */

    /*
     * ...make the output for each file into a single string...
     * (each string being of the same length for use in a columns/rows)
     */
    StringList *filestrings = makefilestrings(filefields, fieldwidths);
    int filewidth = getfilewidth(fieldwidths);
    free(fieldwidths);
    fieldwidths = NULL;
    freelist(filefields, (free_func)freefield);

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

    freelist(filestrings, (free_func)free);
}

/**
 * Print the contents of the given directory.
 */
void listdir(File *dir, Options *poptions)
{
    List *files = newlist();
    if (files == NULL) {
        errorf("files in NULL\n");
        return;
    }
    DIR *openeddir = opendir(dir->path);
    if (openeddir == NULL) {
        errorf("Cannot open %s\n", dir->path);
        return;
    }
    unsigned long totalblocks = 0;
    struct dirent *dirent = NULL;
    while ((dirent = readdir(openeddir)) != NULL) {
        /* TODO: merge this hidden file check with want() */
        if (!poptions->all && dirent->d_name[0] == '.') {
            continue;
        }
        File *file = newfile(dir->name, dirent->d_name);
        if (file == NULL) {
            errorf("file is NULL\n");
            return;
        }
        if (!want(file, poptions)) {
            continue;
        }
        append(file, files);
        if (poptions->dirtotals) {
            totalblocks += getblocks(file, poptions->blocksize);
        }
    }
    closedir(openeddir);

    if (poptions->dirtotals) {
        printf("total %lu\n", totalblocks);
    }
    listfiles(files, poptions);
    freelist(files, (free_func)freefile);
}

/* XXX does C really not provide this? */
char *cescape(char c)
{
    switch (c) {
    case '\001': return "\\001";
    case '\002': return "\\002";
    case '\003': return "\\003";
    case '\004': return "\\004";
    case '\005': return "\\005";
    case '\006': return "\\006";
    case '\007': return "\\a";
    case '\010': return "\\b";
    case '\011': return "\\t";
    case '\012': return "\\n";
    case '\013': return "\\v";
    case '\014': return "\\f";
    case '\015': return "\\r";
    case '\016': return "\\016";
    case '\017': return "\\017";
    case '\020': return "\\020";
    case '\021': return "\\021";
    case '\022': return "\\022";
    case '\023': return "\\023";
    case '\024': return "\\024";
    case '\025': return "\\025";
    case '\026': return "\\026";
    case '\027': return "\\027";
    case '\030': return "\\030";
    case '\031': return "\\031";
    case '\032': return "\\032";
    case '\033': return "\\033";
    case '\034': return "\\034";
    case '\035': return "\\035";
    case '\036': return "\\036";
    case '\037': return "\\037";
    default: return NULL;
    }
}

void printnametobuf(const char *name, Options *poptions, Buf *buf)
{
    if (buf == NULL) {
        errorf("buf is NULL\n");
        return;
    }
    if (name == NULL) {
        errorf("file is NULL\n");
        return;
    }

    const char *p = name;
    for (p = name; *p != '\0'; p++) {
        if (!isprint(*p)) {
            switch (poptions->escape) {
            case ESCAPE_C:
                {
                    char *escaped = cescape(*p);
                    if (escaped == NULL) {
                        errorf("No C escape for %c\n", *p);
                        bufappendchar(buf, *p);
                    } else {
                        bufappend(buf, escaped, strlen(escaped), 1);
                    }
                    break;
                }
            case ESCAPE_QUESTION:
                bufappendchar(buf, '?');
                break;
            default:
                errorf("Unknown escape mode\n");
                /* fall through */
            case ESCAPE_NONE:
                bufappendchar(buf, *p);
                break;
            }
        } else if (*p == '\\' && poptions->escape == ESCAPE_C) {
            bufappendchar(buf, '\\');
            bufappendchar(buf, '\\');
        } else {
            bufappendchar(buf, *p);
        }
    }
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
        errorf("files is NULL\n");
        return;
    }
    if (poptions == NULL) {
        errorf("poptions is NULL\n");
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
        errorf("pcolors is NULL\n");
        return 0;
    }

    char *term = getenv("TERM");
    if (term == NULL) {
        return 0;
    }

    int errret;
    if ((setupterm(term, 1, &errret)) == ERR) {
        errorf("setupterm returned %d\n", errret);
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
    fprintf(stderr, "Usage: %s [-%s] [<file>]...\n", myname, OPTSTRING);
}

/**
 * Returns true if we should print this file,
 * false otherwise.
 */
int want(File *file, Options *poptions)
{
    if (file == NULL) {
        errorf("file is NULL\n");
        return 0;
    } else if (file->path == NULL) {
        errorf("path is NULL\n");
        return 0;
    } else if (poptions == NULL) {
        errorf("poptions is NULL\n");
        return 0;
    }
    /* XXX what to do if we can't lstat the file? */
    return !poptions->dirsonly || isdir(file);
}

/* vim: set ts=4 sw=4 tw=0 et:*/
