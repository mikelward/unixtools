#ifndef FILE_H
#define FILE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "user.h"

typedef struct file {
    char *name;
    char *path;
    int didstat;
    struct stat *pstat;
    struct file *target;           /* holds target file if this file is a symlink */
} File;

typedef int (*file_compare_function)(const File **a, const File **b);

/* create a new File */
File *newfile(const char *dir, const char *name);
/* free any memory held by "file" */
void freefile(File *file);

/* get the name that was supplied when creating the file
   it could be a bare filename or a path
   this is the name that should appear in the output */
const char *getname(File *file);
/* get the full path to file */
const char *getpath(File *file);

/* get the path to file without the last part
   caller should free the returned string if it's not NULL */
char *getdir(File *file);
/* get the last part of the path to file
   caller should free the returned string if it's not NULL */
char *getfile(File *file);

/* caller must free returned path if not NULL */
char *makepath(const char *dirname, const char *filename);

bool isstat(File *file);
bool isdir(File *file);
bool isexec(File *file);
bool islink(File *file);
bool ishidden(File *file);

unsigned long getblocks(File *file, int blocksize);
char *getgroup(File *file);
time_t getmtime(File *file);
ino_t getinode(File *file);
nlink_t getlinkcount(File *file);
char *getmodes(File *file);
char *getowner(File *file);
char *getperms(File *file);
long getsize(File *file);

File *gettarget(File *file);

int comparebyname(const File **a, const File **b);
int comparebymtime(const File **a, const File **b);

struct stat *getstat(File *file);

#endif
/* vim: set ts=4 sw=4 tw=0 et:*/
