#ifndef FILE_H
#define FILE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

typedef struct file {
    char *path;
    struct stat *pstat;
} File;

typedef int (*file_compare_function)(const File **a, const File **b);

/* create a new File */
File *newfile(const char *path);
/* free any memory held by "file" */
void freefile(File *file);

/* caller must free returned filename if not NULL */
char *filename(File *file);
int isdir(File *file);
char *makepath(const char *dirname, const char *filename);

int comparebyname(const File **a, const File **b);
int comparebymtime(const File **a, const File **b);

#endif
/* vim: set ts=4 sw=4 tw=0 et:*/
