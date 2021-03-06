#define _XOPEN_SOURCE 600

#include <assert.h>
#include <stdio.h>
#include <time.h>

#include "buf.h"
#include "field.h"
#include "file.h"
#include "filefields.h"
#include "group.h"
#include "logging.h"
#include "options.h"
#include "string.h"
#include "user.h"

#define HUMANBUFSIZE 64
char humanbuf[HUMANBUFSIZE];

const char *humanbytes(unsigned long bytes);
void printnametobuf(File *file, Options *options, Buf *buf);

FieldList *getfilefields(File *file, Options *options)
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
    if (options->targetinfo && islink(file)) {
        /* XXX ls seems to use stat() instead of lstat() here */
        file = getfinaltarget(file);
        if (!file) {
            errorf("Cannot determine target of %s for %s\n", getname(file));
            /* file is NULL,
               code below should handle this and print "?" or similar */
        }
    }

    if (options->size) {
        Field *field = getsizefield(file, options, snprintfbuf, sizeof(snprintfbuf));
        if (!field) goto error;
        append(field, fieldlist);
    }

    if (options->inode) {
        Field *field = getinodefield(file, options, snprintfbuf, sizeof(snprintfbuf));
        if (!field) goto error;
        append(field, fieldlist);
    }

    if (options->modes) {
        Field *field = getmodesfield(file, options, snprintfbuf, sizeof(snprintfbuf));
        if (!field) goto error;
        append(field, fieldlist);
    }

    if (options->linkcount) {
        Field *field = getlinkfield(file, options, snprintfbuf, sizeof(snprintfbuf));
        if (!field) goto error;
        append(field, fieldlist);
    }

    if (options->owner) {
        Field *field = getownerfield(file, options, snprintfbuf, sizeof(snprintfbuf));
        if (!field) goto error;
        append(field, fieldlist);
    }

    if (options->group) {
        Field *field = getgroupfield(file, options, snprintfbuf, sizeof(snprintfbuf));
        if (!field) goto error;
        append(field, fieldlist);
    }

    if (options->perms) {
        Field *field = getpermsfield(file, options, snprintfbuf, sizeof(snprintfbuf));
        if (!field) goto error;
        append(field, fieldlist);
    }

    if (options->bytes) {
        Field *field = getbytesfield(file, options, snprintfbuf, sizeof(snprintfbuf));
        if (!field) goto error;
        append(field, fieldlist);
    }

    if (options->datetime) {
        Field *field = getdatetimefield(file, options, snprintfbuf, sizeof(snprintfbuf));
        if (!field) goto error;
        append(field, fieldlist);
    }

    Field *field = getnamefield(link, options);
    append(field, fieldlist);

    return (FieldList *)fieldlist;

error:
    freelist(fieldlist, (free_func)freefield);
    return NULL;
}

Field *getbytesfield(File *file, Options *options, char *buf, int bufsize)
{
    int width;
    if (!isstat(file)) {
        width = snprintf(buf, bufsize, "?");
    } else {
        long bytes = getsize(file);
        if (options->sizestyle == SIZE_DEFAULT) {
            width = snprintf(buf, bufsize, "%ld", bytes);
        } else if (options->sizestyle == SIZE_HUMAN) {
            width = snprintf(buf, bufsize, "%s", humanbytes(bytes));
        } else {
            errorf("Unknown sizestyle %d\n", options->sizestyle);
            width = snprintf(buf, bufsize, "%ld", bytes);
        }
    }
    return newfield(buf, ALIGN_RIGHT, width);
}

Field *getdatetimefield(File *file, Options *options, char *buf, int bufsize)
{
    int width;
    if (isstat(file)) {
        time_t timestamp;
        switch (options->timetype) {
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
            errorf("Unknown time attribute\n");
            /* fall through */
        case TIME_MTIME:
            timestamp = getmtime(file);
            break;
        }
        struct tm *timestruct = localtime(&timestamp);
        if (!timestruct) {
            errorf("timestruct is NULL\n");
            return NULL;
        }
        if (options->timeformat != NULL) {
            width = strftime(buf, bufsize, options->timeformat, timestruct);
        } else if (options->timestyle == TIME_RELATIVE) {
            assert(options->now > 0);
            time_t seconds_ago = options->now - timestamp;
            if (seconds_ago > 60*60*24*31*12) {
                width = snprintf(buf, bufsize, "%ld years", seconds_ago/60/60/24/31/12);
            } else if (seconds_ago > 60*60*24*31) {
                width = snprintf(buf, bufsize, "%ld months", seconds_ago/60/60/24/31);
            } else if (seconds_ago > 60*60*24) {
                width = snprintf(buf, bufsize, "%ld days", seconds_ago/60/60/24);
            } else if (seconds_ago > 60*60) {
                width = snprintf(buf, bufsize, "%ld hours", seconds_ago/60/60);
            } else if (seconds_ago > 60) {
                width = snprintf(buf, bufsize, "%ld minutes", seconds_ago/60);
            } else if (seconds_ago >= 0) {
                width = snprintf(buf, bufsize, "%ld seconds", seconds_ago);
            } else {
                width = strftime(buf, bufsize, "%b %e  %Y", timestruct);
            }
        } else {
            /* month day hour and minute if file was modified in the last 6 months,
               month day year otherwise */
            assert(options->now > 0);
            width = strftime(buf, bufsize, "%b %e  %Y", timestruct);
            if (timestamp <= options->now) {
                if (timestamp > options->now - 6*86400) {
                    width = strftime(buf, bufsize, "%a    %H:%M", timestruct);
                } else if (timestamp > options->now - 180*86400) {
                    width = strftime(buf, bufsize, "%b %e %H:%M", timestruct);
                }
            }
        }
    } else {
        width = snprintf(buf, bufsize, "?");
    }
    return newfield(buf, ALIGN_RIGHT, width);
}

Field *getgroupfield(File *file, Options *options, char *buf, int bufsize)
{
    int width;
    if (isstat(file)) {
        gid_t gid = getgroupnum(file);
        if (options->numeric) {
            width = snprintf(buf, bufsize, "%lu", (unsigned long)gid);
        } else {
            char *groupname = get(options->groupnames, gid);
            if (!groupname) {
                groupname = getgroupname(gid);
                if (!groupname) {
                    width = snprintf(buf, bufsize, "%lu", (unsigned long)gid);
                    groupname = buf;
                }
                set(options->groupnames, gid, groupname);
            }
            if (groupname != buf) {
                /* only do this if we didn't already print into the buf */
                width = snprintf(buf, bufsize, "%s", groupname);
            }
        }
    } else {
        width = snprintf(buf, bufsize, "?");
    }
    return newfield(buf, ALIGN_LEFT, width);
}

Field *getinodefield(File *file, Options *options, char *buf, int bufsize)
{
    int width;
    if (isstat(file)) {
        ino_t inode = getinode(file);
        width = snprintf(buf, bufsize, "%lu", inode);
    } else {
        width = snprintf(buf, bufsize, "%s", "?");
    }
    return newfield(buf, ALIGN_RIGHT, width);
}

Field *getlinkfield(File *file, Options *options, char *buf, int bufsize)
{
    int width;
    if (isstat(file)) {
        nlink_t nlinks = getlinkcount(file);
        width = snprintf(buf, bufsize, "%lu", (unsigned long)nlinks);
    } else {
         width = snprintf(buf, bufsize, "%s", "?");
    }
    return newfield(buf, ALIGN_RIGHT, width);
}

Field *getmodesfield(File *file, Options *options, char *buf, int bufsize)
{
    int width;
    if (isstat(file)) {
        char *modes = getmodes(file);
        width = snprintf(buf, bufsize, "%s", modes);
        free(modes);
    } else {
        width = snprintf(buf, bufsize, "%s", "???????????");
    }
    return newfield(buf, ALIGN_LEFT, width);
}

/**
 * Print the file name with flags and colors if requested
 * flags, color start, filename, color end, and flags are made into a single field
 * since we don't want spaces in between
 * if -L is given and the file is a symlink, the returned fields include the name
 * of the link's target (multiple times if there are multiple links)
 * i.e. link -> file
 * or even link -> link -> link -> file
 *
 * A symlink loop will be printed as
 * link1 -> link2 -> link3 -> link1
 */
Field *getnamefield(File *file, Options *options)
{
    if (!file) {
        errorf("file is NULL\n");
        return NULL;
    }
    if (!options) {
        errorf("options is NULL\n");
        return NULL;
    }

    Buf *buf = newbuf();
    if (!buf) {
        errorf("buf is NULL\n");
        return NULL;
    }

    /* print the file itself... */
    printnametobuf(file, options, buf);

    if (options->showlinks) {
        Map *linkmap = newmap();
        if (linkmap) {
            /* resolve and print symlink targets recursively */
            while (isstat(file) && islink(file)) {
                if (inmap(linkmap, getinode(file))) {
                    /* error already printed by getfilefields */
                    /* no file to stat, but want to print the name field */
                    file = NULL;
                    break;
                } else {
                    set(linkmap, (uintmax_t)getinode(file), NULL);
                }
                file = gettarget(file);
                bufappend(buf, " -> ", 4, 4);
                printnametobuf(file, options, buf);
            }
            freemap(linkmap);
        } else {
            errorf("Out of memory?\n");
        }
    } else if (options->showlink) {
        /* print only the first link target without stat'ing the target */
        if (isstat(file) && islink(file)) {
            file = gettarget(file);
            bufappend(buf, " -> ", 4, 4);
            printnametobuf(file, options, buf);
        }
    }

    enum align align = ALIGN_NONE;
    if (options->displaymode != DISPLAY_ONE_PER_LINE) {
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

Field *getownerfield(File *file, Options *options, char *buf, int bufsize)
{
    int width;
    if (isstat(file)) {
        uid_t uid = getownernum(file);
        if (options->numeric) {
            width = snprintf(buf, bufsize, "%lu", (unsigned long)uid);
        } else {
            char *username = get(options->usernames, uid);
            if (!username) {
                username = getusername(uid);
                if (!username) {
                    width = snprintf(buf, bufsize, "%lu", (unsigned long)uid);
                    username = buf;
                }
                set(options->usernames, uid, username);
            }
            if (username != buf) {
                width = snprintf(buf, bufsize, "%s", username);
            }
        }
    } else {
        width = snprintf(buf, bufsize, "?");
    }
    return newfield(buf, ALIGN_LEFT, width);
}

Field *getpermsfield(File *file, Options *options, char *buf, int bufsize)
{
    char *perms = getperms(file);
    if (perms == NULL) {
        errorf("perms is NULL\n");
        return NULL;
    }
    int width = strlen(perms);
    Field *field = newfield(perms, ALIGN_RIGHT, width);
    free(perms);
    return field;
}

Field *getsizefield(File *file, Options *options, char *buf, int bufsize)
{
    int width;
    if (!isstat(file)) {
        width = snprintf(buf, bufsize, "%s", "?");
    } else {
        unsigned long blocks = getblocks(file, options->blocksize);
        if (options->sizestyle == SIZE_DEFAULT) {
            width = snprintf(buf, bufsize, "%lu", blocks);
        } else if (options->sizestyle == SIZE_HUMAN) {
            unsigned long bytes = blocks * options->blocksize;
            width = snprintf(buf, bufsize, "%s", humanbytes(bytes));
        } else {
            errorf("Unknown sizestyle %d", options->sizestyle);
            width = snprintf(buf, bufsize, "%lu", blocks);
        }
    }
    return newfield(buf, ALIGN_RIGHT, width);
}

void printnametobuf(File *file, Options *options, Buf *buf)
{
    assert(file != NULL);
    assert(options != NULL);
    assert(buf != NULL);

    /*
     * print a character *before* the file showing its type (-O)
     *
     * early versions of BSD printed directories like [this]
     */
    switch (options->flags) {
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
    if (options->color) {
        if (!isstat(file)) {
            bufappend(buf, options->colors->red, strlen(options->colors->red), 0);
            colorused = 1;
        } else if (isdir(file)) {
            bufappend(buf, options->colors->blue, strlen(options->colors->blue), 0);
            colorused = 1;
        } else if (islink(file)) {
            bufappend(buf, options->colors->cyan, strlen(options->colors->cyan), 0);
            colorused = 1;
        } else if (isexec(file)) {
            bufappend(buf, options->colors->green, strlen(options->colors->green), 0);
            colorused = 1;
        }
    }

    const char *name = getname(file);
    printtobuf(name, options->escape, buf);
    /* don't free name */

    /* reset the color back to normal (-G and -K) */
    if (colorused) {
        bufappend(buf, options->colors->none, strlen(options->colors->none), 0);
    }

    /* print a character after the file showing its type (-F and -O) */
    switch (options->flags) {
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

const char *humanbytes(unsigned long bytes)
{
    char *units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB", NULL};
    int i;
    for (i = 0; bytes >= 1000 && units[i] != NULL; i++) {
        bytes /= 1000;
    }
    snprintf(humanbuf, sizeof(humanbuf), "%ld %s", bytes, units[i]);
    return humanbuf;
}
