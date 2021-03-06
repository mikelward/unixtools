#define _XOPEN_SOURCE 600   /* for readlink(), strdup(), snprintf() */
#define _GNU_SOURCE         /* for strverscmp() */

#include <sys/stat.h>
#include <sys/param.h>      /* for DEV_BSIZE */
#include <sys/acl.h>
#include <errno.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "logging.h"
#include "map.h"

struct file {
    char *name;
    char *path;
    int didstat;
    struct stat *pstat;
    struct file *target;           /* holds target file if this file is a symlink */
};

/**
 * A File.
 *
 * We need to be able to print the file name as it was supplied, e.g.
 * if the command line argument was /tmp/foo, that's what we print in
 * the output.  Use getname(file) for that.
 *
 * We also need a file's full path so that we can pass the correct argument
 * to lstat, etc when listing something other than the current directory.
 * Use getpath(file) for that.
 *
 * There are also getdirname(file) and getbasename(file), which
 * work like POSIX basename() and dirname().
 */

/**
 * Return a File object for the given filename.
 */
File *newfile(const char *dir, const char *name)
{
    if (!dir) {
        errorf("dir is NULL\n");
        return NULL;
    }
    if (!name) {
        errorf("name is NULL\n");
        return NULL;
    }

    File *file = malloc(sizeof *file);
    if (!file) {
        errorf("Out of memory\n");
        return NULL;
    }

    char *namecopy = strdup(name);
    if (!namecopy) {
        errorf("namecopy is NULL\n");
        free(file);
        return NULL;
    }
    file->name = namecopy;

    char *path = makepath(dir, name);
    if (!path) {
        errorf("path is NULL\n");
        free(namecopy);
        free(file);
        return NULL;
    }
    file->path = path;

    /* to be filled in as and when required */
    /* calling code must check if these are NULL */
    file->didstat = 0;
    file->pstat = NULL;
    file->target = NULL;

    return file;
}

/**
 * Free all memory allocated by newfile.
 */
void freefile(File *file)
{
    if (!file) return;

    free(file->name);
    free(file->path);
    free(file->pstat);
    freefile(file->target);
    free(file);
}

const char *getname(File *file)
{
    if (!file) {
        errorf("file is NULL\n");
        return NULL;
    }
    //char *namecopy = strdup(file->name);
    //return namecopy;
    return file->name;
}

const char *getpath(File *file)
{
    if (!file) {
        errorf("file is NULL\n");
        return NULL;
    }

    //char *pathcopy = strdup(file->path);
    //return pathcopy;
    return file->path;
}

char *getbasename(File *file)
{
    if (!file) {
        errorf("file is NULL\n");
        return NULL;
    }

   /*
    * uses icky basename() function
    * I'm following the POSIX version according to the Linux man pages
    * thus I have to take a copy of the path
    */
    char *pathcopy = strdup(file->path);
    char *base = basename(pathcopy);
    char *namecopy = strdup(base);
    free(pathcopy);
    return namecopy;
}

char *getdirname(File *file)
{
    if (!file) {
        errorf("file is NULL\n");
        return NULL;
    }

    if (!file->path) {
        errorf("file->path is NULL\n");
        return NULL;
    }
    char *pathcopy = strdup(file->path);
    char *dir = dirname(pathcopy);
    char *dircopy = strdup(dir);
    free(pathcopy);
    return dircopy;
}

/**
 * Fill the pstat field if it's not already populated and return it.
 */
static struct stat *getstat(File *file)
{
    if (!file) {
        errorf("file is NULL\n");
        return NULL;
    }

    if (!file->didstat) {
        struct stat *pstat = malloc(sizeof(*pstat));
        if (!pstat) {
            errorf("Out of memory\n");
            return NULL;
        }
        errno = 0;
        file->didstat = 1;
        if (lstat(file->path, pstat) != 0) {
            errorf("Cannot lstat %s: %s\n", file->path, strerror(errno));
            free(pstat);
            return NULL;
        }
        file->pstat = pstat;
    }

    return file->pstat;
}

bool isstat(File *file)
{
    if (!file) {
        return 0;
    }

    return getstat(file);
}

bool isblockdev(File *file)
{
    struct stat *pstat = getstat(file);
    return pstat && S_ISBLK(pstat->st_mode);
}

bool ischardev(File *file)
{
    struct stat *pstat = getstat(file);
    return pstat && S_ISCHR(pstat->st_mode);
}

bool isdir(File *file)
{
    struct stat *pstat = getstat(file);
    return pstat && S_ISDIR(pstat->st_mode);
}

bool isexec(File *file)
{
    struct stat *pstat = getstat(file);
    return pstat && pstat->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH);
}

bool isfifo(File *file)
{
    struct stat *pstat = getstat(file);
    return pstat && S_ISFIFO(pstat->st_mode);
}

bool islink(File *file)
{
    struct stat *pstat = getstat(file);
    return pstat && S_ISLNK(pstat->st_mode);
}

bool issetgid(File *file)
{
    struct stat *pstat = getstat(file);
    return pstat && pstat->st_mode & S_ISGID;
}

bool issetuid(File *file)
{
    struct stat *pstat = getstat(file);
    return pstat && pstat->st_mode & S_ISUID;
}

bool issock(File *file)
{
    struct stat *pstat = getstat(file);
    return pstat && S_ISSOCK(pstat->st_mode);
}

bool issticky(File *file)
{
    struct stat *pstat = getstat(file);
    return pstat && pstat->st_mode & S_ISVTX;
}

char *makepath(const char *dirname, const char *filename)
{
    char *path = NULL;
    unsigned size;

    if (!dirname || !*dirname) {
        return strdup(filename);
    }
    if (!filename) {
        return strdup(dirname);
    }
    if (filename[0] == '/') {
        return strdup(filename);
    }

    size = snprintf(path, 0, "%s/%s", dirname, filename);
    if (size < 1) {
        errorf("snprintf is not C99 compliant?\n");
        return NULL;
    }
    size += 1;                  /* allow for the null byte */
    path = malloc(size);
    if (!path) {
        errorf("Out of memory\n");
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

    return strcoll(fa->name, fb->name);
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
int comparebyatime(const File **a, const File **b)
{
    File *fa = *(File **)a;
    File *fb = *(File **)b;

    struct stat *psa = getstat(fa);
    struct stat *psb = getstat(fb);

    if (psa->st_atime == psb->st_atime) {
        return strcoll(fa->name, fb->name);
    } else {
        return psa->st_atime < psb->st_atime;
    }
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
int comparebyctime(const File **a, const File **b)
{
    File *fa = *(File **)a;
    File *fb = *(File **)b;

    struct stat *psa = getstat(fa);
    struct stat *psb = getstat(fb);

    if (psa->st_ctime == psb->st_ctime) {
        return strcoll(fa->name, fb->name);
    } else {
        return psa->st_ctime < psb->st_ctime;
    }
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
        return strcoll(fa->name, fb->name);
    } else {
        return psa->st_mtime < psb->st_mtime;
    }
}

/*
 * returns:
 * -1 if a should come before b (a is larger than b)
 *  0 if a is the same size as b
 *  1 if a should come after b (a is smaller than b)
 *
 *  this is backwards from normal,
 *  but by default ls shows largest files first,
 *  so it's what we want here
 */
int comparebyblocks(const File **a, const File **b)
{
    File *fa = *(File **)a;
    File *fb = *(File **)b;

    struct stat *psa = getstat(fa);
    struct stat *psb = getstat(fb);

    if (psa->st_blocks == psb->st_blocks) {
        return strcoll(fa->name, fb->name);
    } else {
        return psa->st_blocks < psb->st_blocks;
    }
}

/*
 * returns:
 * -1 if a should come before b (a is larger than b)
 *  0 if a is the same size as b
 *  1 if a should come after b (a is smaller than b)
 *
 *  this is backwards from normal,
 *  but by default ls shows largest files first,
 *  so it's what we want here
 */
int comparebysize(const File **a, const File **b)
{
    File *fa = *(File **)a;
    File *fb = *(File **)b;

    struct stat *psa = getstat(fa);
    struct stat *psb = getstat(fb);

    if (psa->st_size == psb->st_size) {
        return strcoll(fa->name, fb->name);
    } else {
        return psa->st_size < psb->st_size;
    }
}

/*
 * returns:
 * -1 if a should come before b (a's version is lower than b's)
 *  0 if a is the same size as b
 *  1 if a should come after b (a's version is higher than b's)
 */
int comparebyversion(const File **a, const File **b)
{
    File *fa = *(File **)a;
    File *fb = *(File **)b;

    return strverscmp(fa->name, fb->name);
}

unsigned long getblocks(File *file, int blocksize)
{
    struct stat *pstat = getstat(file);
    if (!pstat) return 0;
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

time_t getatime(File *file)
{
    struct stat *pstat = getstat(file);
    return pstat && pstat->st_atime;
}

/*
time_t getbtime(File *file)
{
    struct stat *pstat = getstat(file);
    return pstat && pstat->st_birthtime;
}
*/

time_t getctime(File *file)
{
    struct stat *pstat = getstat(file);
    return pstat && pstat->st_ctime;
}

/* TODO return the number as a string or "?" if the group could not be determined */
gid_t getgroupnum(File *file)
{
    struct stat *pstat = getstat(file);
    if (!pstat) return 0;
    return pstat->st_gid;
}

/* TODO return a string, "?" if stat fails */
nlink_t getlinkcount(File *file)
{
    struct stat *pstat = getstat(file);
    if (!pstat) return 0;
    return pstat->st_nlink;
}

char *getmodes(File *file)
{
    char *unknownmodes = "???????????";
    char *modes = malloc((strlen(unknownmodes) + 1) * sizeof(*modes));
    if (!modes) {
        return strdup(unknownmodes);
    }
    struct stat *pstat = getstat(file);
    if (!pstat) {
        free(modes);
        return strdup(unknownmodes);
    }

    char *p = modes;
    if (islink(file))
        *p++ = 'l';
    else if (isdir(file))
        *p++ = 'd';
    else if (isblockdev(file))
        *p++ = 'b';
    else if (ischardev(file))
        *p++ = 'c';
    else if (isfifo(file))
        *p++ = 'p';
    else if (issock(file))
        *p++ = 's';
    else
        *p++ = '-';

    if (pstat->st_mode & S_IRUSR)
        *p++ = 'r';
    else
        *p++ = '-';

    if (pstat->st_mode & S_IWUSR)
        *p++ = 'w';
    else
        *p++ = '-';

    if (issetuid(file)) {
        if (pstat->st_mode & S_IXUSR)
            *p++ = 's';
        else
            *p++ = 'S';
    } else {
        if (pstat->st_mode & S_IXUSR)
            *p++ = 'x';
        else
            *p++ = '-';
    }

    if (pstat->st_mode & S_IRGRP)
        *p++ = 'r';
    else
        *p++ = '-';

    if (pstat->st_mode & S_IWGRP)
        *p++ = 'w';
    else
        *p++ = '-';

    if (issetgid(file)) {
        if (pstat->st_mode & S_IXGRP)
            *p++ = 's';
        else
            *p++ = 'S';
    } else {
        if (pstat->st_mode & S_IXGRP)
            *p++ = 'x';
        else
            *p++ = '-';
    }

    if (pstat->st_mode & S_IROTH)
        *p++ = 'r';
    else
        *p++ = '-';

    if (pstat->st_mode & S_IWOTH)
        *p++ = 'w';
    else
        *p++ = '-';

    if (issticky(file)) {
        if (pstat->st_mode & S_IXOTH)
            *p++ = 't';
        else
            *p++ = 'T';
    } else {
        if (pstat->st_mode & S_IXOTH)
            *p++ = 'x';
        else
            *p++ = '-';
    }

    /* POSIX says we should print a space if there are no extended ACLs,
       GNU ls prints nothing
       follow POSIX */
    if (!islink(file) && hasacls(file))
        *p++ = '+';
    else
        *p++ = ' ';

    *p++ = '\0';
    return modes;
}

/* TODO return a string, "?" on error */
ino_t getinode(File *file)
{
    struct stat *pstat = getstat(file);
    if (!pstat) return 0;
    return pstat->st_ino;
}

/* TODO return a string, "?" on error */
time_t getmtime(File *file)
{
    struct stat *pstat = getstat(file);
    if (!pstat) return 0;
    return pstat->st_mtime;
}

/* caller must free */
char *getperms(File *file)
{
    char *unknownperms = "???";
    if (!file) return strdup(unknownperms);

    char *path = file->path;
    char *perms, *p;
    perms = malloc((strlen(unknownperms) + 1) * sizeof(*perms));
    if (!perms) {
        errorf("Out of memory\n");
        return NULL;
    }
    p = perms;

    if (access(path, R_OK) == 0)
        (*p++) = 'r';
    else if (errno == EACCES)
        (*p++) = '-';
    else
        (*p++) = '?';

    if (access(path, W_OK) == 0)
        (*p++) = 'w';
    else if (errno == EACCES)
        (*p++) = '-';
    else
        (*p++) = '?';

    if (access(path, X_OK) == 0)
        (*p++) = 'x';
    else if (errno == EACCES)
        (*p++) = '-';
    else
        (*p++) = '?';

    *p = '\0';
    return perms;
}

/* TODO return a string instead of uid_t, "?" on failure */
uid_t getownernum(File *file)
{
    struct stat *stat = getstat(file);
    if (!stat) return 0;
    return stat->st_uid;
}

/* TODO return a string instead of long, "?" on failure */
long getsize(File *file)
{
    struct stat *pstat = getstat(file);
    if (!pstat) return -1;
    return (long)pstat->st_size;
}

File *gettarget(File *file)
{
    if (!file) {
        errorf("file is NULL\n");
        return NULL;
    }
    if (!islink(file)) {
        errorf("%s is not a symlink\n", file->path);
        return NULL;
    }
    if (!file->target) {
        char targetpath[PATH_MAX];
        /* note: readlink(3p) not readlink(2) */
        errno = 0;
        int nchars = readlink(file->path, targetpath, sizeof(targetpath)-1);
        if (nchars == -1) {
            errorf("Error getting symlink target: %s\n", strerror(errno));
            return NULL;
        } else if (nchars == sizeof(targetpath)-1) {
            errorf("Symlink target too long for buffer\n");
            return NULL;
        }
        targetpath[nchars] = '\0';
        char *dir = getdirname(file);
        if (dir == NULL) {
            errorf("getdirname returned NULL\n");
            return NULL;
        }
        file->target = newfile(dir, targetpath);
        free(dir);
        if (file->target == NULL) {
            errorf("newfile returned NULL\n");
            return NULL;
        }
    }
    return file->target;
}

File *getfinaltarget(File *file)
{
    Map *linkmap = newmap();
    if (!linkmap) {
        errorf("Out of memory?\n");
        return NULL;
    }
    File *target = NULL;
    while (isstat(file) && islink(file)) {
        target = gettarget(file);
        if (!target) {
            errorf("Cannot determine target of %s for %s\n", getname(file));
            break;
        }
        if (inmap(linkmap, getinode(target))) {
            errorf("Symlink loop in %s\n", getname(file));
            /* no file to stat, but want to print the name field */
            target = NULL;
            break;
        } else {
            set(linkmap, (uintmax_t)getinode(target), NULL);
        }
        file = target;
    }
    freemap(linkmap);
    return target;
}

/*
 * return true if file has ACLs beyond the traditional Unix owner/group/other ACLs
 */
bool hasacls(File *file)
{
    acl_type_t acl_types[] = { ACL_TYPE_ACCESS, ACL_TYPE_DEFAULT };
    for (int i = 0; i < sizeof(acl_types)/sizeof(acl_types[0]); i++) {
        if (!isdir(file) && acl_types[i] == ACL_TYPE_DEFAULT) continue;

        errno = 0;
        acl_t acl = acl_get_file(file->path, acl_types[i]);
        // XXX is this valid?
        if (!acl) {
            if (errno == EOPNOTSUPP) {
                /* file system does not support ACLs */
                return false;
            }
            errorf("Error getting ACLs for %s: %s\n", file->name, strerror(errno));
            // error = 1;
            break;
        }
        bool extended_found = 0;
        //bool error = 0;
        for (int entry_id = ACL_FIRST_ENTRY; ; entry_id = ACL_NEXT_ENTRY) {
            acl_entry_t entry;
            errno = 0;
            int status = acl_get_entry(acl, entry_id, &entry);
            if (status == -1) {
                errorf("Error getting ACLs for %s: %s\n", file->name, strerror(errno));
                // XXX return -1 or '?' or something
                //error = 1;
                break;
            } else if (status == 0) {
                /* no more ACL entries */
                break;
            }
            acl_tag_t tag_type;
            status = acl_get_tag_type(entry, &tag_type);
            switch (tag_type) {
            case ACL_USER_OBJ: case ACL_GROUP_OBJ: case ACL_OTHER:
                break;
            default:
                extended_found = 1;
                break;
            }
            /* no acl_free_entry? */
            if (extended_found) break;
        }
        acl_free(acl);
        if (extended_found) return 1;
    }

    return 0;
}

/* vim: set ts=4 sw=4 tw=0 et:*/
