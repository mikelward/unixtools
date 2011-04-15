#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"

int main(int argc, const char *argv[])
{
    int i;
    List *pl = newlist();

    for (i = 0; i < 10000; i++) {
        int *pi = malloc(sizeof(int));
        *pi = i;
        append(pi, pl);
    }

    for (i = 0; i < length(pl); i++) {
        int *pi = (int *)getitem(pl, i);
        printf("%d\n", *pi);
    }

    return 0;
}
/* vim: set ts=4 sw=4 tw=0 et:*/
