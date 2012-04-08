#define _XOPEN_SOURCE 600   /* for readlink(), strdup(), snprintf() */

#include <sys/stat.h>
#include <sys/param.h>      /* for DEV_BSIZE */
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "logging.h"

File *newfile(const char *path)
{
    if (path == NULL) {
        errorf(__func__, "path is NULL\n");
        return NULL;
    }

    File *file = malloc(sizeof *file);
    if (file == NULL) {
        errorf(__func__, "Out of memory\n");
        return NULL;
    }

    char *copy = strdup(path);
    if (copy == NULL) {
        errorf(__func__, "copy is NULL\n");
        return NULL;
    }
    file->path = copy;

    /* to be filled in as and when required */
    /* calling code must check if these are NULL */
    file->pstat = NULL;
    file->target = NULL;

    return file;
}

void freefile(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return;
    }

    if (file->path) {
        free(file->path);
        file->path = NULL;
    }
    if (file->pstat) {
        free(file->pstat);
        file->pstat = NULL;
    }
    File *target = file->target;
    while (target != NULL) {
        File *nexttarget = target->target;
        freefile(target);
        target = nexttarget;
    }
}

/*
 * uses icky basename() function
 * I'm following the POSIX version according to the Linux man pages
 * thus I have to take a copy of the path
 *
 * caller should free the returned string
 */
char *getname(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return NULL;
    }

    char *pathcopy = strdup(file->path);
    char *base = basename(pathcopy);
    char *name = strdup(base);
    free(pathcopy);
    return name;
}

/*
 * fill the pstat field if it's not already populated
 */
struct stat *getstat(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return NULL;
    }

    if (file->pstat == NULL) {
        struct stat *pstat = malloc(sizeof(*pstat));
        if (pstat == NULL) {
            errorf(__func__, "Out of memory\n");
            return NULL;
        }
        if (lstat(file->path, pstat) != 0) {
            errorf(__func__, "Cannot lstat %s\n", file->path);
            return NULL;
        }
        file->pstat = pstat;
    }

    return file->pstat;
}

int isdir(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return 0;
    }

    struct stat *pstat = getstat(file);
    if (pstat == NULL) {
        errorf(__func__, "pstat is NULL\n");
        return 0;
    }

    return S_ISDIR(pstat->st_mode);
}

int isexec(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return 0;
    }

    struct stat *pstat = getstat(file);
    if (pstat == NULL) {
        errorf(__func__, "pstat is NULL\n");
        return 0;
    }

    return pstat->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH);
}

int islink(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return 0;
    }

    struct stat *pstat = getstat(file);
    if (pstat == NULL) {
        errorf(__func__, "pstat is NULL\n");
        return 0;
    }

    return S_ISLNK(pstat->st_mode);
}

char *makepath(const char *dirname, const char *filename)
{
    char *path = NULL;
    unsigned size;

    size = snprintf(path, 0, "%s/%s", dirname, filename);
    if (size < 1) {
        errorf(__func__, "snprintf is not C99 compliant?\n");
        return NULL;
    }
    size += 1;                  /* allow for the null byte */
    path = malloc(size);
    if (path == NULL) {
        errorf(__func__, "Out of memory\n");
        return NULL;
    }
    size = snprintf(path, size, "%s/%s", dirname, filename);
    return path;
}

/*
 * returns:
 * -1 if a's path should come before a's
 *  0 if the paths are the same
 *  1 if a's path should come after b's
 */
int comparebyname(const File **a, const File **b)
{
    File *fa = *(File **)a;
    File *fb = *(File **)b;

    return strcoll(fa->path, fb->path);
}

/*
 * returns:
 * -1 if a should come before b (a is newer than b)
 *  0 if a is the same age as b
 *  1 if a should come after b (a is older than b)
 *
 *  this is backwards from normal,
 *  but by default ls shows newest files first,
 *  so it's what we want here
 */
int comparebymtime(const File **a, const File **b)
{
    File *fa = *(File **)a;
    File *fb = *(File **)b;

    struct stat *psa = getstat(fa);
    struct stat *psb = getstat(fb);

    if (psa->st_mtime == psb->st_mtime) {
        return strcoll(fa->path, fb->path);
    } else {
        return psa->st_mtime < psb->st_mtime;
    }
}

unsigned long getblocks(File *file, int blocksize)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return 0;
    }
    struct stat *pstat = getstat(file);
    if (pstat == NULL) {
        errorf(__func__, "pstat is NULL\n");
        return 0;
    }
    unsigned long blocks = pstat->st_blocks;
    /* blocks are stored as an unsigned long on i686
     * when dealing with integral types, we have to do
     * larger / smaller to avoid getting zero for everything
     * but we also want to do the blocksize/BSIZE calculation
     * first to reduce the chances of overflow */
    if (blocksize > DEV_BSIZE) {
        int factor = blocksize / DEV_BSIZE;
        /* round up to nearest integer
         * e.g. if it takes 3 * 512 byte blocks,
         * it would take 2 1024 byte blocks */
        return (blocks+(factor/2)) / factor;
    }
    else {
        int factor = DEV_BSIZE / blocksize;
        return blocks * factor;
    }
}

ino_t getinode(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return 0;
    }
    struct stat *pstat = getstat(file);
    if (pstat == NULL) {
        errorf(__func__, "pstat is NULL\n");
        return 0;
    }
    return pstat->st_ino;
}

char *getmymodes(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return 0;
    }
    char *path = file->path;
    char *pbuf, *p;

    pbuf = malloc(4 * sizeof(*pbuf));
    if (pbuf == NULL) {
        errorf(__func__, "pbuf is NULL\n");
        return NULL;
    }
    p = pbuf;

    if (access(path, R_OK) == 0)
        (*p++) = 'r';
    else
        (*p++) = '-';

    if (access(path, W_OK) == 0)
        (*p++) = 'w';
    else
        (*p++) = '-';

    if (access(path, X_OK) == 0)
        (*p++) = 'x';
    else
        (*p++) = '-';

    *p = '\0';
    return pbuf;
}

File *gettarget(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return NULL;
    }
    if (!islink(file)) {
        errorf(__func__, "%s is not a symlink\n", file->path);
        return NULL;
    }
    if (file->target == NULL) {
        char targetpath[PATH_MAX];
        /* note: readlink(3p) not readlink(2) */
        errno = 0;
        int nchars = readlink(file->path, targetpath, sizeof(targetpath)-1);
        if (nchars == -1) {
            errorf(__func__, "Error getting symlink target: %s\n", strerror(errno));
            return NULL;
        } else if (nchars == sizeof(targetpath)-1) {
            errorf(__func__, "Symlink target too long for buffer\n");
            return NULL;
        }
        targetpath[nchars] = '\0';
        file->target = newfile(targetpath);
        if (file->target == NULL) {
            errorf(__func__, "newfile returned NULL\n");
            return NULL;
        }
    }
    return file->target;
}

/* vim: set ts=4 sw=4 tw=0 et:*/
