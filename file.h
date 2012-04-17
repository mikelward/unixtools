#ifndef FILE_H
#define FILE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

#include "user.h"

typedef struct file {
    char *path;
    int didstat;
    struct stat *pstat;
    struct file *target;           /* holds target file if this file is a symlink */
} File;

typedef int (*file_compare_function)(const File **a, const File **b);

/* create a new File */
File *newfile(const char *path);
/* free any memory held by "file" */
void freefile(File *file);

int isstat(File *file);
int isdir(File *file);
int isexec(File *file);
int islink(File *file);
/* caller must free returned path if not NULL */
char *makepath(const char *dirname, const char *filename);
unsigned long getblocks(File *file, int blocksize);
ino_t getinode(File *file);
char *getmymodes(File *file);
char *getowner(File *file);
/* caller must free returned name if not NULL */
char *getname(File *file);
char *getpath(File *file);
File *gettarget(File *file);

int comparebyname(const File **a, const File **b);
int comparebymtime(const File **a, const File **b);

struct stat *getstat(File *file);

#endif
/* vim: set ts=4 sw=4 tw=0 et:*/
