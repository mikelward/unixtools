#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"

void test1(void);

int main(int argc, const char *argv[])
{
    test1();

    return 0;
}

void test1(void)
{
    int i;
    List *pl = newlist();

    for (i = 0; i < 10; i++) {
        int *pi = malloc(sizeof(int));
        *pi = i;
        append(pi, pl);
    }

    for (i = 0; i < length(pl); i++) {
        int *pi = (int *)getitem(pl, i);
        assert(*pi == i);
    }
}

/* vim: set ts=4 sw=4 tw=0 et:*/
