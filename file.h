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

/**
 * Free any memory held by file.
 */
void freefile(File *file);

/**
 * Create a new File.
 */
File *newfile(const char *dir, const char *name);

/**
 * Get the name that was supplied when creating the file.
 *
 * It could be a bare filename or a path.
 *
 * This is the name that should appear in the output.
 *
 * Caller should NOT free the returned string.
 */
const char *getname(File *file);

/**
 * Get the full path to file, like realpath().
 *
 * Caller should NOT free the returned string.
 *
 * @see realpath()
 */
const char *getpath(File *file);

/**
 * Get the last part of the path to file, like basename().
 *
 * Caller should free the returned string.
 *
 * @see basename()
 */
char *getbasename(File *file);

/**
 * Get the all but the last part of the path to file, like dirname().
 *
 * Caller should free the returned string.
 *
 * @see dirname()
 */
char *getdirname(File *file);

/**
 * Create a full path from dirname and filename.
 *
 * Caller must free returned string.
 */
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
 * Modes will have a "+" suffix if file has extended ACLs
 * and a " " suffix if file has only minimal ACLs.
 *
 * Caller must free returned string.
 */
char *getmodes(File *file);

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
int comparebyblocks(const File **a, const File **b);
int comparebysize(const File **a, const File **b);

#endif
/* vim: set ts=4 sw=4 tw=0 et:*/
