#ifndef FILE_H
#define FILE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

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

/* get the last part of the path to file
   caller should free the returned string if it's not NULL */
char *getbasename(File *file);
/* get the path to file without the last part
   caller should free the returned string if it's not NULL */
char *getdirname(File *file);

/* caller must free returned path if not NULL */
char *makepath(const char *dirname, const char *filename);

bool isstat(File *file);
bool isblockdev(File *file);
bool ischardev(File *file);
bool isdir(File *file);
bool isexec(File *file);
bool isfifo(File *file);
bool islink(File *file);
bool issetgid(File *file);
bool issetuid(File *file);
bool issock(File *file);
bool issticky(File *file);
bool ishidden(File *file);

bool hasacls(File *file);

unsigned long getblocks(File *file, int blocksize);
/**
 * Return the name of the group of the file.
 *
 * Caller must NOT free returned string.
 */
char *getgroupname(File *file);
gid_t getgroupnum(File *file);
time_t getatime(File *file);
//time_t getbtime(File *file);
time_t getctime(File *file);
time_t getmtime(File *file);
ino_t getinode(File *file);
nlink_t getlinkcount(File *file);
/**
 * Return modes as a string, e.g. "-rwxr-xr-x"
 *
 * Caller must free returned string.
 */
char *getmodes(File *file);
char *getownername(File *file);
uid_t getownernum(File *file);
/**
 * Return access permissions for current user, e.g. "rwx".
 *
 * Caller must free returned string.
 */
char *getperms(File *file);
long getsize(File *file);

File *gettarget(File *file);

int comparebyname(const File **a, const File **b);
int comparebyatime(const File **a, const File **b);
int comparebyctime(const File **a, const File **b);
int comparebymtime(const File **a, const File **b);

struct stat *getstat(File *file);

#endif
/* vim: set ts=4 sw=4 tw=0 et:*/
