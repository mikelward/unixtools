/*
 * simple version of ls
 *
 * TODO:
 * handling of symlinks (use lstat()?)
 * several options
 */

#define _BSD_SOURCE             /* for strdup() */

#include <sys/types.h>
#include <sys/param.h>          /* for DEV_BSIZE */
#include <sys/stat.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>             /* for getopt() */
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "list.h"

struct options {
    int all : 1;
    int directory : 1;
    int one : 1;
    int size : 1;
    int blocksize;
    int (*sort)(const void *, const void *);
};
typedef struct options Options;

struct file {
    const char *path;       /* full path to the file */
    const char *name;       /* file name without leading directories */
    struct stat *psb;
};
typedef struct file File;

int sortbyname(const void *a, const void *b)
{
    const File *fa = *(File **)a;
    const File *fb = *(File **)b;
    const char *namea = fa->path;
    const char *nameb = fb->path;

    //return strcmp(sa, sb);
    return strcoll(namea, nameb);
}

int sortdir(const void *a, const void *b)
{
    char *namea = *(char **)a;
    char *nameb = *(char **)b;

    return strcoll(namea, nameb);
}

/* naive implementation; not benchmarked */
/*******************************
 * LAST ARGUMENT MUST BE NULL  *
 * to signify end of arguments *
 ******************************/
char *strconcat(const char *s1, ...)
{
    int len, newlen;
    char *ret, *rettmp;
    va_list ap;

    va_start(ap, s1);

    const char *s;
    ret = NULL;
    len = 0;
    for (s = s1;
         s != NULL;
         s = va_arg(ap, const char *)) {
         newlen = len + strlen(s);
         rettmp = realloc(ret, newlen+1);
         if (!rettmp) {
             fprintf(stderr, "Error realloc'ing string\n");
             free(ret);
             return NULL;
         }
         ret = rettmp;
         strcpy(ret+len, s);
         len = newlen;
    }
    return ret;
}

/*
 * makepath("/dir", "file") => "/dir/file"
 */
char *makepath(const char *dirname, const char *filename)
{
    if (dirname && *dirname && filename && *filename) {
        return strconcat(dirname, "/", filename, NULL);
    }
    else {
        return strconcat(dirname, filename, NULL);
    }
}


/*
 * return a new struct file pointer with information about "path"
 *
 * caller must free the returned structure when it's done with it
 *
 * stat() is only called if "wantstat" is true (e.g. 1)
 * 
 * if "wantstat" is false (0) or stat() fails, the psb member will be NULL
 * callers should check for this condition before using psb
 */
File *getfile(const char *dirname, const char *filename, int wantstat)
{
    File *pfile = malloc(sizeof *pfile);
    if (!pfile) {
        fprintf(stderr, "Out of memory\n");
        return NULL;
    }

    pfile->path = makepath(dirname, filename);
    pfile->name = strdup(filename);
    pfile->psb = NULL;

    if (wantstat) {
        struct stat *psb = malloc(sizeof *psb);
        if (psb) {
            if (stat(pfile->path, psb) == 0) {
                pfile->psb = psb;
            }
            else {
                fprintf(stderr, "Error getting file status for %s: %s\n",
                        pfile->path, strerror(errno));
            }
        }
    }

    return pfile;
}

/*
 * returns 1 if poptions mean we will need to call stat() to produce the correct output
 * returns 0 if poptions means we do not require stat()
 */
int needfilestat(Options *poptions)
{
    return poptions->size;
}

/*
 * like needfilestat(), but if the -d flag is not given,
 * we also need to stat a path to see if it's a directory or not
 */
int needdirstat(Options *poptions)
{
    return !poptions->directory || needfilestat(poptions);
}


int isdir(File *fp)
{
    return S_ISDIR(fp->psb->st_mode);
}

int wantthisfile(const char *filename, Options *poptions)
{
    return poptions->all || (filename[0] != '.');
}

unsigned long getdisplaysize(blkcnt_t blocks, Options *poptions)
{

    /* yep, a file that's only using one block gets printed as size "0"!
     * that's what GNU ls does anyway */
    unsigned long size;
    /* contortions to attempt to avoid overflow */
    /* in both cases we want size = blocks * BSIZE / blocksize */
    /* and in both cases we have to have larger / smaller
     * to avoid truncating the answer down to zero */
    if (DEV_BSIZE > poptions->blocksize) {
        size = blocks * (DEV_BSIZE / poptions->blocksize);
    } else {
        size = blocks / (poptions->blocksize / DEV_BSIZE);
    }
    return size;
}

void listfile(File *fp, Options *poptions, int fullpath)
{
    if (poptions->size) {
        printf("%6lu ", getdisplaysize(fp->psb->st_blocks, poptions)); 
    }

    const char *name = (fullpath)? fp->path: fp->name;

    printf("%s\n", name);
}

void listdir(File *fp, Options *poptions)
{
    if (!fp) {
        fprintf(stderr, "listdir: fp is NULL\n");
        return;
    }
    if (!poptions) {
        fprintf(stderr, "listdir: poptions is NULL\n");
        return;
    }

    const char *dirname = fp->path;
    errno = 0;
    DIR *pdir = opendir(dirname);
    if (pdir) {
        List *files = newlist();
        blkcnt_t totalblocks = 0;
        int wantstat = needfilestat(poptions);
        struct dirent *pde;
        while ((pde = readdir(pdir))) {
            if (!wantthisfile(pde->d_name, poptions)) {
                continue;
            }
            File *file = getfile(dirname, pde->d_name, wantstat);
            if (!file) {
                fprintf(stderr, "Error getting file\n");
                continue;
            }
            append(file, files);

            if (poptions->size) {
                totalblocks += file->psb->st_blocks;
            }
        }
        if (poptions->size) {
            printf("total %lu\n", getdisplaysize(totalblocks, poptions));
        }
        if (poptions->sort) {
            sort(files, poptions->sort);
        }
        unsigned nfiles = length(files);
        for (int i = 0; i < nfiles; i++) {
            File *file = getitem(files, i);
            listfile(file, poptions, 0);
        }
        free(files);
    }
    else {
        fprintf(stderr, "Error opening directory %s: %s\n", dirname, strerror(errno));
    }
}

int main(int argc, char *argv[])
{
    /* so that files are sorted the same as GNU ls */
    setlocale(LC_ALL, "");

    Options options;
    options.all = 0;
    options.blocksize = 1024;       /* bytes per block */
    options.directory = 0;
    options.one = 1;                /* XXX maybe change later? */
    options.size = 0;
    options.sort = &sortbyname;

    int option;
    opterr = 0;
    while ((option = getopt(argc, argv, ":1adsU")) != -1) {
        switch (option) {
        case '1':
            options.one = 1;
            break;
        case 'a':
            options.all = 1;
            break;
        case 'd':
            options.directory = 1;
            break;
        case 's':
            options.size = 1;
            break;
        case 'U':
            options.sort = NULL;
            break;
        case ':':
            fprintf(stderr, "Missing argument to -%c\n", optopt);
            exit(2);
        case '?':
            fprintf(stderr, "Invalid option -%c\n", optopt);
            exit(2);
        default:
            fprintf(stderr, "Unsupported option -%c\n", option);
            exit(2);
        }
    }
    argc -= optind, argv += optind;

    List *files = newlist();
    List *dirs = newlist();

    /* list current directory if no other paths were given */
    if (argc == 0) {
        argc = 1, argv[0] = ".";
    }

    for (int i = 0; i < argc; i++) {
        /* XXX should dirs be recorded is dir=dir, file="", or something else? */
        /* (maybe it should depend on whether it contains a slash,
         *  maybe dirname/basename should be used?) */
        /* current rule:
         * path is always set to the full path,
         * name is set if we are listing its directory */
        char *path = argv[i];
        File *file = getfile(path, "", 1);
        if (!file || !file->psb) {
            continue;
        }
        else if (!options.directory && isdir(file)) {
            append(file, dirs);
        }
        else {
            append(file, files);
        }
    }

    int nprinted = 0;

    /* print files first, sorted according to user preference */
    if (options.sort && length(files) > 0) {
        sort(files, options.sort);
    }
    for (int i = 0; i < length(files); i++) {
        File *file = getitem(files, i);
        listfile(file, &options, 1);
        nprinted++;
    }

    /* then print directories, sorted by name */
    if (options.sort && length(dirs) > 0) {
        sort(dirs, &sortbyname);
    }
    for (int i = 0; i < length(dirs); i++) {
        File *dir = getitem(dirs, i);
        if (nprinted > 0) {
            printf("\n");
        }
        if (nprinted > 0 || length(dirs) > 1) {
            printf("%s:\n", dir->path);
        }
        listdir(dir, &options);
        nprinted++;
    }

    return 0;
}

/* vim: set ts=4 sw=4 tw=0 et:*/
