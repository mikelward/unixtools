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
    }
    if (file->path) {
        free(file->path);
    }
    free(file);
}

/*
 * uses icky basename() function
 * I'm following the POSIX version according to the Linux man pages
 * this I have to take a copy of the path
 */
char *filename(File *file)
{
    if (file == NULL) {
        fprintf(stderr, "filename: file is NULL\n");
        return NULL;
    }

    char *pathcopy = strdup(file->path);
    char *name = strdup(basename(pathcopy));
    free(pathcopy);
    return name;
}

int isdir(File *file)
{
    if (file == NULL) {
        fprintf(stderr, "isdir: file is NULL\n");
        return 0;
    }

    if (file->pstat == NULL) {
        struct stat *pstat = malloc(sizeof(pstat));
        if (pstat == NULL) {
            fprintf(stderr, "isdir: Out of memory\n");
            return 0;
        }
        if (stat(file->path, pstat) != 0) {
            fprintf(stderr, "isdir: Cannot stat %s\n", file->path);
            return 0;
        }
        file->pstat = pstat;
    }

    return S_ISDIR(file->pstat->st_mode);
}

int comparebyname(const File **a, const File **b)
{
    File *fa = *(File **)a;
    File *fb = *(File **)b;

    return strcoll(fa->path, fb->path);
}

/* vim: set ts=4 sw=4 tw=0 et:*/
