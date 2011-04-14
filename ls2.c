#define _BSD_SOURCE             /* for strdup() */

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

int strcmpforqsort(const void *a, const void *b)
{
    const char *sa = *(char **)a;
    const char *sb = *(char **)b;

    //return strcmp(sa, sb);
    return strcoll(sa, sb);
}

void ls(const char *root)
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
            char **names;
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

            qsort(names, i, sizeof(char *), strcmpforqsort);
            for (int j = 0; j < i; j++) {
                if (strchr(names[j], '.') == names[j]) {
                    /* skip dot files by default */
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

int main(int argc, const char *argv[])
{
    /* so that files are sorted the same as GNU ls */
    setlocale(LC_ALL, "");

    /* list current directory if no other paths were given */
    if (argc == 1) {
        ls(".");
    }
    else {
        for (int i = 1; i < argc; i++) {
            ls(argv[i]);
        }
    }

    return 0;
}

/* vim: set ts=4 sw=4 tw=0 et:*/
