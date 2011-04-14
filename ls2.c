/*
 * simple version of ls
 *
 * TODO:
 * handling of symlinks (use lstat()?)
 * several options
 */

#define _BSD_SOURCE             /* for strdup() */

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>             /* for getopt() */
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct options {
    int (*sort)(const void *, const void *);
    int all : 1;
    int directory : 1;
};

struct file {
    const char *filename;
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
struct file *getfile(const char *path, int wantstat)
{
    struct file *pfile = malloc(sizeof *pfile);
    char *pathcopy = strdup(path);
    pfile->filename = pathcopy;
    pfile->psb = NULL;

    if (wantstat) {
        struct stat *psb = malloc(sizeof *psb);
        if (psb) {
            if (stat(path, psb) == 0) {
                pfile->psb = psb;
            }
            else {
                fprintf(stderr, "Error getting file status for %s\n", path);
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
    return 0;
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
            struct file *file = getfile(pde->d_name, wantstat);
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
 * by default, if "path" is a directory, its contents will be listed
 * the -d flag causes only the directory itself to be listed
 */
void ls(const char *path, struct options *poptions)
{
    int wantdirstat = needdirstat(poptions);
    struct file *fp = getfile(path, wantdirstat);
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
    options.directory = 0;
    options.sort = &sortbyname;

    int option;
    opterr = 0;
    while ((option = getopt(argc, argv, ":ad")) != -1) {
        switch (option) {
        case 'a':
            options.all = 1;
            break;
        case 'd':
            options.directory = 1;
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
