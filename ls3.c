/*
 * simple reimplementation of ls
 *
 * TODO
 * - most flags
 * - handling of symlink arguments
 */

#define _POSIX_C_SOURCE 200809L /* for getopt() opt* variables */

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "list.h"
#include "file.h"

/* typedefs to make function pointers readable */
typedef int (*file_compare_function)(const File **a, const File **b);
typedef int (*qsort_compare_function)(const void *a, const void *b);

/* all the command line options */
typedef struct options {
    int all : 1;
    int directory : 1;
    int one : 1;
    file_compare_function compare;
} Options;

void listfile(File *file, Options *poptions);
void listfiles(List *files, Options *poptions);
void listdir(File *dir, Options *poptions);
void sortfiles(List *files, Options *poptions);
void usage(void);
int want(const char *path, Options *poptions);

int main(int argc, char **argv)
{
    /* so that file names are sorted according to the user's locale */
    setlocale(LC_ALL, "");

    Options options;
    options.all = 0;
    options.directory = 0;
    options.one = 1;
    options.compare = &comparebyname;

    opterr = 0;     /* we will print our own error messages */
    int option;
    while ((option = getopt(argc, argv, ":1ad")) != -1) {
        switch(option) {
        case '1':
            options.one = 1;
            break;
        case 'a':
            options.all = 1;
            break;
        case 'd':
            options.directory = 1;
            break;
        case ':':
            fprintf(stderr, "Missing argument to -%c\n", optopt);
            usage();
            exit(2);
        case '?':
            fprintf(stderr, "Unknown option -%c\n", optopt);
            usage();
            exit(2);
        default:
            usage();
            exit(2);
        }
    }

    /* skip program name and flags */ 
    argc -= optind, argv += optind;

    if (argc == 0) {
        argv[0] = ".";
        argc = 1;
    }

    /*
     * ls handles command lines differently depending on
     * whether they are directories or regular files.
     * regular files are listed first, then all directories follow
     */
    List *files = newlist();
    if (files == NULL) {
        fprintf(stderr, "ls3: files is NULL\n");
        exit(1);
    }
    List *dirs = newlist();
    if (dirs == NULL) {
        fprintf(stderr, "ls3: dirs is NULL\n");
        exit(1);
    }
    for (int i = 0; i < argc; i++) {
        File *file = newfile(argv[i]);
        if (file == NULL) {
            fprintf(stderr, "ls3: file is NULL\n");
            exit(1);
        }
        if (isdir(file)) {
            append(file, dirs);
        } else {
            append(file, files);
        }
    }

    listfiles(files, &options);
 
    for (int i = 0, ndirs = length(dirs); i < ndirs; i++) {
        File *dir = getitem(dirs, i);
        if (dir == NULL) {
            fprintf(stderr, "ls3: dir is NULL\n");
            continue;
        }
        listdir(dir, &options);
        free(dir);
    }

    free(files);
    free(dirs);
}

void listfile(File *file, Options *poptions)
{
    char *name = filename(file);
    if (name == NULL) {
        fprintf(stderr, "ls3: file is NULL\n");
        return;
    }
    printf("%s\n", name);
    free(name);
}

void listfiles(List *files, Options *poptions)
{
    if (files == NULL) {
        fprintf(stderr, "listfiles: files is NULL\n");
        return;
    } else if (poptions == NULL) {
        fprintf(stderr, "listfiles: poptions is NULL\n");
        return;
    }

    /* files are sorted according to user preference...*/
    sortfiles(files, poptions);

    for (int i = 0, nfiles = length(files); i < nfiles; i++) {
        File *file = getitem(files, i);
        if (file == NULL) {
            fprintf(stderr, "listfiles: file is NULL\n");
            continue;
        }
        listfile(file, poptions);
    }
}

void listdir(File *dir, Options *poptions)
{
    List *files = newlist();
    if (files == NULL) {
        fprintf(stderr, "listdir: files in NULL\n");
        return;
    }
    DIR *openeddir = opendir(dir->path);
    if (openeddir == NULL) {
        fprintf(stderr, "listdir: Cannot open %s\n", dir->path);
        return;
    }
    struct dirent *dirent;
    while ((dirent = readdir(openeddir)) != NULL) {
        if (!want(dirent->d_name, poptions)) {
            continue;
        }
        File *file = newfile(dirent->d_name);
        if (file == NULL) {
            fprintf(stderr, "listdir: file is NULL\n");
            return;
        }
        append(file, files);
    }

    listfiles(files, poptions);
}

void sortfiles(List *files, Options *poptions)
{
    /* our sort function takes two File **s
     * qsort has to declare itself to take void pointers
     * so it can work with any type
     * cast our File ** function to void * to keep the compiler happy */
    qsort_compare_function qcompare = (qsort_compare_function)poptions->compare;

    sort(files, qcompare);
}

void usage(void)
{
    fprintf(stderr, "Usage: ls3 [-1ad] <file>...\n");
}

int want(const char *path, Options *poptions)
{
    if (path == NULL) {
        fprintf(stderr, "want: path is NULL\n");
        return 0;
    } else if (poptions == NULL) {
        fprintf(stderr, "want: poptions is NULL\n");
        return 0;
    }
    return poptions->all || path[0] != '.';
}

/* vim: set ts=4 sw=4 tw=0 et:*/
