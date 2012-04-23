#define _XOPEN_SOURCE 600   /* for readlink(), strdup(), snprintf() */

#include <sys/stat.h>
#include <sys/param.h>      /* for DEV_BSIZE */
#include <sys/acl.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "group.h"
#include "logging.h"

/*
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
 */

File *newfile(const char *dir, const char *name)
{
    if (dir == NULL) {
        errorf(__func__, "dir is NULL\n");
        return NULL;
    }
    if (name == NULL) {
        errorf(__func__, "name is NULL\n");
        return NULL;
    }

    File *file = malloc(sizeof *file);
    if (file == NULL) {
        errorf(__func__, "Out of memory\n");
        return NULL;
    }

    char *namecopy = strdup(name);
    if (namecopy == NULL) {
        errorf(__func__, "namecopy is NULL\n");
        return NULL;
    }
    file->name = namecopy;

    char *path = makepath(dir, name);
    if (path == NULL) {
        errorf(__func__, "path is NULL\n");
        return NULL;
    }
    file->path = path;

    /* to be filled in as and when required */
    /* calling code must check if these are NULL */
    file->didstat = 0;
    file->pstat = NULL;
    file->target = NULL;

    /*
    char *d = getdir(file);
    char *f = getfile(file);
    errorf(__func__, "new file: name=%s, dir=%s, file=%s, path=%s\n",
                      file->name, d, f, file->path);
    free(d);
    free(f);
    */

    return file;
}

void freefile(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return;
    }

    if (file->name != NULL) {
        free(file->name);
        file->name = NULL;
    }
    if (file->path != NULL) {
        free(file->path);
        file->path = NULL;
    }
    if (file->pstat != NULL) {
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

const char *getname(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return NULL;
    }
    //char *namecopy = strdup(file->name);
    //return namecopy;
    return file->name;
}

const char *getpath(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return NULL;
    }

    //char *pathcopy = strdup(file->path);
    //return pathcopy;
    return file->path;
}

/*
 * uses icky basename() function
 * I'm following the POSIX version according to the Linux man pages
 * thus I have to take a copy of the path
 *
 * caller should free the returned string
 */
char *getfile(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return NULL;
    }

    char *pathcopy = strdup(file->path);
    char *base = basename(pathcopy);
    char *namecopy = strdup(base);
    free(pathcopy);
    return namecopy;
}

/*
 * caller should free the returned string
 */
char *getdir(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return NULL;
    }

    if (file->path == NULL) {
        errorf(__func__, "file->path is NULL\n");
        return NULL;
    }
    char *pathcopy = strdup(file->path);
    char *dir = dirname(pathcopy);
    char *dircopy = strdup(dir);
    free(pathcopy);
    return dircopy;
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

    if (!file->didstat) {
        struct stat *pstat = malloc(sizeof(*pstat));
        if (pstat == NULL) {
            errorf(__func__, "Out of memory\n");
            return NULL;
        }
        errno = 0;
        file->didstat = 1;
        if (lstat(file->path, pstat) != 0) {
            errorf(__func__, "Cannot lstat %s: %s\n", file->path, strerror(errno));
            return NULL;
        }
        file->pstat = pstat;
    }

    return file->pstat;
}

bool isstat(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return 0;
    }

    return getstat(file) != NULL;
}

bool isblockdev(File *file)
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

    return S_ISBLK(pstat->st_mode);
}

bool ischardev(File *file)
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

    return S_ISCHR(pstat->st_mode);
}

bool isdir(File *file)
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

bool isexec(File *file)
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

bool isfifo(File *file)
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

    return S_ISFIFO(pstat->st_mode);
}

bool islink(File *file)
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

bool issetgid(File *file)
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

    return pstat->st_mode & S_ISGID;
}

bool issetuid(File *file)
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

    return pstat->st_mode & S_ISUID;
}

bool issock(File *file)
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

    return S_ISSOCK(pstat->st_mode);
}

bool issticky(File *file)
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

    return pstat->st_mode & S_ISVTX;
}

char *makepath(const char *dirname, const char *filename)
{
    char *path = NULL;
    unsigned size;

    if (dirname == NULL || strcmp(dirname, ".") == 0) {
        return strdup(filename);
    }
    if (filename == NULL) {
        return strdup(dirname);
    }
    if (filename[0] == '/') {
        return strdup(filename);
    }

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

char *getgroup(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return NULL;
    }
    struct stat *stat = getstat(file);
    if (stat == NULL) {
        errorf(__func__, "stat is NULL\n");
        return NULL;
    }

    Group *group = newgroup(stat->st_gid);
    if (group == NULL) {
        // TODO snprintf("%u", stat->st_gid);
    }

    return getgroupname(group);
}

// TODO return a string instead of gid_t?
// return '?' instead of 0 on failure?
gid_t getgroupnum(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return 0;
    }
    struct stat *stat = getstat(file);
    if (stat == NULL) {
        errorf(__func__, "stat is NULL\n");
        return 0;
    }

    return stat->st_gid;
}

nlink_t getlinkcount(File *file)
{
    struct stat *pstat = getstat(file);
    if (pstat == NULL) {
        errorf(__func__, "pstat is NULL\n");
        return 0;
    }

    return pstat->st_nlink;
}

char *getmodes(File *file)
{
    /* e.g. -rwxr-xr-x\0 */
    char *modes = malloc(11);
    if (modes == NULL) {
        errorf(__func__, "out of memory\n");
        return NULL;
    }

    struct stat *pstat = getstat(file);
    if (pstat == NULL) {
        errorf(__func__, "pstat is NULL\n");
        return NULL;
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

    if (hasacls(file))
        *p++ = '+';
    else
        *p++ = '.';

    *p++ = '\0';
    return modes;
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

time_t getmtime(File *file)
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
    return pstat->st_mtime;
}

char *getperms(File *file)
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
    return pbuf;
}

char *getowner(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return NULL;
    }
    struct stat *stat = getstat(file);
    if (stat == NULL) {
        errorf(__func__, "stat is NULL\n");
        return NULL;
    }

    User *user = newuser(stat->st_uid);
    if (user == NULL) {
        // TODO snprintf("%u", stat->st_uid);
    }

    // TODO free user
    return getusername(user);
}

// TODO return a string instead of uid_t?
// return '?' instead of 0 on failure?
uid_t getownernum(File *file)
{
    if (file == NULL) {
        errorf(__func__, "file is NULL\n");
        return 0;
    }
    struct stat *stat = getstat(file);
    if (stat == NULL) {
        errorf(__func__, "stat is NULL\n");
        return 0;
    }

    return stat->st_uid;
}

long getsize(File *file)
{
    struct stat *pstat = getstat(file);
    if (pstat == NULL) {
        errorf(__func__, "pstat is NULL\n");
        return -1;
    }

    return (long)pstat->st_size;
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
        char *dir = getdir(file);
        if (dir == NULL) {
            errorf(__func__, "getdir returned NULL\n");
            return NULL;
        }
        file->target = newfile(dir, targetpath);
        free(dir);
        if (file->target == NULL) {
            errorf(__func__, "newfile returned NULL\n");
            return NULL;
        }
    }
    return file->target;
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
        if (acl == (acl_t)NULL) {
            errorf(__func__, "Error getting ACLs for %s: %s\n", file->name, strerror(errno));
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
                errorf(__func__, "Error getting ACLs for %s: %s\n", file->name, strerror(errno));
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
