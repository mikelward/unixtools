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
    int all : 1;
    int (*sort)(const void *, const void *);
};

int sortbyname(const void *a, const void *b)
{
    const char *sa = *(char **)a;
    const char *sb = *(char **)b;

    //return strcmp(sa, sb);
    return strcoll(sa, sb);
}

void ls(const char *root, struct options *poptions)
{
    struct stat buf;

    errno = 0;
    if (stat(root, &buf) == -1) {
        fprintf(stderr, "ls2: cannot access %s: %s\n", root, strerror(errno));
        return;
    }

    if (S_ISDIR(buf.st_mode)) {
        errno = 0;
        DIR *pdir = opendir(root);
        if (pdir) {
            char **names = NULL;
            int namesmax = 0;
            int i = 0;
            struct dirent *pde;
            while ((pde = readdir(pdir))) {
                if (i == namesmax) {
                    char **newnames = realloc(names, (namesmax+=256)*sizeof(*newnames));
                    if (!newnames) {
                        fprintf(stderr, "Error allocating memory\n");
                        free(names);
                        return;
                    }
                    names = newnames;
                }
                names[i++] = strdup(pde->d_name);
            }

            if (poptions->sort) {
                qsort(names, i, sizeof(char *), poptions->sort);
            }
            for (int j = 0; j < i; j++) {
                if ((poptions->all == 0) &&
                    (strchr(names[j], '.') == names[j])) {
                    /* skip dot files */
                    continue;
                }
                printf("%s\n", names[j]);
            }
            free(names);
        }
    }
    else {
        printf("%s\n", root);
    }
}

int main(int argc, char *argv[])
{
    /* so that files are sorted the same as GNU ls */
    setlocale(LC_ALL, "");

    struct options options;
    options.all = 0;
    options.sort = &sortbyname;

    int option;
    opterr = 0;
    while ((option = getopt(argc, argv, ":a")) != -1) {
        switch (option) {
        case 'a':
            options.all = 1;
            break;
        case ':':
            fprintf(stderr, "Missing argument to -%c\n", optopt);
            exit(2);
        case '?':
            fprintf(stderr, "Invalid option -%c\n", optopt);
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
