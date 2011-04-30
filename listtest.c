#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"

void test1(void);
void test2(void);
void test3(void);
void test4(void);

int main(int argc, const char *argv[])
{
    test1();
    test2();
    test3();
    test4();

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

int compareints(const void *pi1, const void *pi2)
{
    int i1 = *(int *)pi1;
    int i2 = *(int *)pi2;

    /* XXX can overflow */
    return i1 - i2;
}

int compareintptrs(const void **ppi1, const void **ppi2)
{
    int i1 = **(int **)ppi1;
    int i2 = **(int **)ppi2;

    /* XXX can overflow */
    return i1 - i2;
}

void test2(void)
{
    List *pl = newlist();
    int is[] = { 3, 1, 4, 1, 5, 2, 9 };
    int i;
    size_t nelems = sizeof(is) / sizeof(is[0]);

    for (i = 0; i < nelems; i++) {
        int *pi = malloc(sizeof *pi);
        *pi = is[i];
        append(pi, pl);
    }

    sortlist(pl, &compareintptrs);
    qsort(is, nelems, sizeof(*is), &compareints);

    for (i = 0; i < nelems; i++) {
        int *pi = getitem(pl, i);
        assert(*pi == is[i]);
    }
}

void sum(void *pnumber, void *context)
{
    int *ptotal = (int *)context;
    int value = *(int *)pnumber;

    *ptotal += value;
}

void test3(void)
{
    List *pl = newlist();
    int i;
    int total = 0;

    for (i = 1; i <= 3; i++) {
        int *pi = malloc(sizeof *pi);
        *pi = i;
        append(pi, pl);
    }

    walklist(pl, 1, &sum, &total);
    //printf("total = %d\n", total);
    assert(total == 6);
}

void printintptr(void *elem, void *context)
{
    int *pi = (int *)elem;
    printf("%d\n", *pi);
}

void checkbackwards(void *elem, void *voidlist)
{
    static int n = 0;

    List *list = (List *)voidlist;
    int *pi = (int *)elem;
    unsigned size = length(list);
    int *pj = (int *)getitem(list, size-1-n);

    //printf("element %d is %d\n", size-1-n, *pj);
    assert(*pi == *pj);

    n++;
}

void test4(void)
{
    List *pl = newlist();
    int i;

    for (i = 1; i <= 3; i++) {
        int *pi = malloc(sizeof *pi);
        *pi = i;
        append(pi, pl);
    }

    walklist(pl, -1, &checkbackwards, pl);
}

/* vim: set ts=4 sw=4 tw=0 et:*/
