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

struct options {
    int all : 1;
    int directory : 1;
    int size : 1;
    int blocksize;
    int (*sort)(const void *, const void *);
};

struct file {
    const char *path;       /* full path to the file */
    const char *filename;   /* path to the file without leading directories */
    struct stat *psb;
};

int sortbyname(const void *a, const void *b)
{
    const struct file *fa = *(struct file **)a;
    const struct file *fb = *(struct file **)b;
    const char *namea = fa->filename;
    const char *nameb = fb->filename;

    //return strcmp(sa, sb);
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
    return strconcat(dirname, "/", filename, NULL);
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
struct file *getfile(const char *dirname, const char *filename, int wantstat)
{
    struct file *pfile = malloc(sizeof *pfile);
    if (!pfile) {
        return NULL;
    }

    pfile->path = makepath(dirname, filename);
    pfile->filename = strdup(filename);
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
int needfilestat(struct options *poptions)
{
    return poptions->size;
}

/*
 * like needfilestat(), but if the -d flag is not given,
 * we also need to stat a path to see if it's a directory or not
 */
int needdirstat(struct options *poptions)
{
    return !poptions->directory || needfilestat(poptions);
}


int isdir(struct file *fp)
{
    return S_ISDIR(fp->psb->st_mode);
}

int wantthisfile(struct file *fp, struct options *poptions)
{
    return poptions->all || (fp->filename[0] != '.');
}

void listfile(struct file *fp, struct options *poptions)
{
    if (poptions->size) {
        /* yep, a file that's only using one block gets printed as size "0"!
         * that's what ls does */
        unsigned long size;
        /* contortions to attempt to avoid overflow */
        /* in both cases we want size = blocks * BSIZE / blocksize */
        /* and in both cases we have to have larger / smaller
         * to avoid truncating the answer down to zero */
        if (DEV_BSIZE > poptions->blocksize) {
            size = fp->psb->st_blocks * (DEV_BSIZE / poptions->blocksize);
        } else {
            size = fp->psb->st_blocks / (poptions->blocksize / DEV_BSIZE);
        }
        printf("%6lu ", size); 
    }

    printf("%s\n", fp->filename);
}

void listdir(struct file *fp, struct options *poptions)
{
    if (!fp) {
        fprintf(stderr, "listdir: fp is NULL\n");
        return;
    }
    if (!poptions) {
        fprintf(stderr, "listdir: poptions is NULL\n");
        return;
    }

    const char *dirname = fp->filename;
    errno = 0;
    DIR *pdir = opendir(dirname);
    if (pdir) {
        struct file **files = NULL;
        int filessize = 0;
        int filesinc = 256;
        int nfiles = 0;
        int wantstat = needfilestat(poptions);
        struct dirent *pde;
        while ((pde = readdir(pdir))) {
            if (nfiles == filessize) {
                struct file **newfiles = realloc(files, (filessize+=filesinc)*sizeof(*files));
                if (!newfiles) {
                    fprintf(stderr, "Error allocating memory\n");
                    free(files);
                    return;
                }
                files = newfiles;
            }
            struct file *file = getfile(dirname, pde->d_name, wantstat);
            if (!file) {
                fprintf(stderr, "Error getting file\n");
                continue;
            }
            files[nfiles++] = file;
        }

        if (poptions->sort) {
            qsort(files, nfiles, sizeof(struct file *), poptions->sort);
        }
        for (int i = 0; i < nfiles; i++) {
            if (!wantthisfile(files[i], poptions)) {
                continue;
            }
            listfile(files[i], poptions);
        }
        free(files);
    }
    else {
        fprintf(stderr, "Error opening directory %s: %s\n", dirname, strerror(errno));
    }
}

/*
 * called for each file specified on the command line
 *
 * by default, if "file" is a directory, its contents will be listed
 * the -d flag causes only the directory itself to be listed
 */
void ls(const char *file, struct options *poptions)
{
    int wantdirstat = needdirstat(poptions);
    struct file *fp = getfile("", file, wantdirstat);
    if (!fp) {
        return;
    }

    if (poptions->directory) {
        listfile(fp, poptions);
    }
    else {
        if (isdir(fp)) {
            listdir(fp, poptions);
        }
        else {
            listfile(fp, poptions);
        }
    }

}

int main(int argc, char *argv[])
{
    /* so that files are sorted the same as GNU ls */
    setlocale(LC_ALL, "");

    struct options options;
    options.all = 0;
    options.blocksize = 1024;       /* bytes per block */
    options.directory = 0;
    options.size = 0;
    options.sort = &sortbyname;

    int option;
    opterr = 0;
    while ((option = getopt(argc, argv, ":ads")) != -1) {
        switch (option) {
        case 'a':
            options.all = 1;
            break;
        case 'd':
            options.directory = 1;
            break;
        case 's':
            options.size = 1;
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

    /* list current directory if no other paths were given */
    if (argc == 0) {
        ls(".", &options);
    }
    else {
        for (int i = 0; i < argc; i++) {
            ls(argv[i], &options);
        }
    }

    return 0;
}

/* vim: set ts=4 sw=4 tw=0 et:*/
