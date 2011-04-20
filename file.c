#define _XOPEN_SOURCE 600   /* for strdup() */

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"

File *newfile(const char *path)
{
    if (path == NULL) {
        fprintf(stderr, "newfile: path is NULL\n");
        return NULL;
    }

    File *file = malloc(sizeof *file);
    if (file == NULL) {
        fprintf(stderr, "newfile: Out of memory\n");
        return NULL;
    }

    char *copy = strdup(path);
    if (copy == NULL) {
        fprintf(stderr, "newfile: copy is NULL\n");
        return NULL;
    }
    file->path = copy;

    /* to be filled in as and when required */
    /* calling code must check if this is NULL */
    file->pstat = NULL;

    return file;
}

void freefile(File *file)
{
    if (file == NULL) {
        fprintf(stderr, "freefile: file is NULL\n");
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
    free(file);
}

/*
 * uses icky basename() function
 * I'm following the POSIX version according to the Linux man pages
 * thus I have to take a copy of the path
 */
char *filename(File *file)
{
    if (file == NULL) {
        fprintf(stderr, "filename: file is NULL\n");
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
        fprintf(stderr, "getstat: file is NULL\n");
        return NULL;
    }

    if (file->pstat == NULL) {
        struct stat *pstat = malloc(sizeof(*pstat));
        if (pstat == NULL) {
            fprintf(stderr, "getstat: Out of memory\n");
            return NULL;
        }
        if (stat(file->path, pstat) != 0) {
            fprintf(stderr, "getstat: Cannot stat %s\n", file->path);
            return NULL;
        }
        file->pstat = pstat;
    }

    return file->pstat;
}

int isdir(File *file)
{
    if (file == NULL) {
        fprintf(stderr, "isdir: file is NULL\n");
        return 0;
    }

    struct stat *pstat = getstat(file);
    if (pstat == NULL) {
        fprintf(stderr, "isdir: pstat is NULL\n");
        return 0;
    }

    return S_ISDIR(file->pstat->st_mode);
}

char *makepath(const char *dirname, const char *filename)
{
    char *path = NULL;
    unsigned size;

    size = snprintf(path, 0, "%s/%s", dirname, filename);
    if (size < 1) {
        fprintf(stderr, "makepath: snprintf is not C99 compliant?\n");
        return NULL;
    }
    size += 1;                  /* allow for the null byte */
    path = malloc(size);
    if (path == NULL) {
        fprintf(stderr, "makepath: Out of memory\n");
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

/* vim: set ts=4 sw=4 tw=0 et:*/
