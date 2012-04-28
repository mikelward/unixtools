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

void printnametobuf(File *file, Options *options, Buf *buf);

Field *getbytesfield(File *file, Options *options, char *buf, int bufsize)
{
    int width;
    if (isstat(file)) {
        long bytes = getsize(file);
        width = snprintf(buf, bufsize, "%ld", bytes);
    } else {
        width = snprintf(buf, bufsize, "?");
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
        } else {
            /* month day hour and minute if file was modified in the last 6 months,
               month day year otherwise */
            if (options->now != -1 && timestamp <= options->now && timestamp > options->now - 180*86400) {
                width = strftime(buf, bufsize, "%b %e %H:%M", timestruct);
            } else {
                width = strftime(buf, bufsize, "%b %e  %Y", timestruct);
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
 */
Field *getnamefield(File *file, Options *options)
{
    if (file == NULL) {
        errorf("file is NULL\n");
        return NULL;
    }
    if (options == NULL) {
        errorf("options is NULL\n");
        return NULL;
    }

    Buf *buf = newbuf();
    if (buf == NULL) {
        errorf("buf is NULL\n");
        return NULL;
    }

    /* print the file itself... */
    printnametobuf(file, options, buf);

    if (options->showlinks) {
        /* resolve and print symlink targets recursively */
        while (isstat(file) && islink(file)) {
            file = gettarget(file);
            bufappend(buf, " -> ", 4, 4);
            printnametobuf(file, options, buf);
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
    if (isstat(file)) {
        unsigned long blocks = getblocks(file, options->blocksize);
        width = snprintf(buf, bufsize, "%lu", blocks);
    } else {
        width = snprintf(buf, bufsize, "%s", "?");
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
