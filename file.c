#define _BSD_SOURCE

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

int comparebyname(const File **a, const File **b)
{
    File *fa = *(File **)a;
    File *fb = *(File **)b;

    return strcoll(fa->path, fb->path);
}

/* vim: set ts=4 sw=4 tw=0 et:*/