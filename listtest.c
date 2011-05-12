#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"

void test1(void);
void test2(void);
void test3(void);
void test4(void);
void test5(void);
void test6(void);
void test7(void);
void test8(void);
void test9(void);
void test10(void);

int main(int argc, const char *argv[])
{
    test1();
    test2();
    test3();
    //test4();
    test5();
    test6();
    test7();
    //test8();
    test9();
    test10();

    return 0;
}

void test1(void)
{
    int i;
    List *pl = newlist();

    fprintf(stderr, "%s\n", __func__);

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

    fprintf(stderr, "%s\n", __func__);

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

    fprintf(stderr, "%s\n", __func__);

    for (i = 1; i <= 3; i++) {
        int *pi = malloc(sizeof *pi);
        *pi = i;
        append(pi, pl);
    }

    walklist(pl, &sum, &total);
    //printf("total = %d\n", total);
    assert(total == 6);
}

void printintptr(void *elem, void *context)
{
    int *pi = (int *)elem;
    printf("%d\n", *pi);
}

void donothing(void *elem, void *context)
{
}

/*
 * pass in the element AND the whole list
 * figure out which element we expect,
 * get that element from the list,
 * and assert that elem == that element
 */
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

/*
 * backwards support temporarily removed to simplify things
void test4(void)
{
    List *pl = newlist();
    int i;

    fprintf(stderr, "%s\n", __func__);

    for (i = 1; i <= 3; i++) {
        int *pi = malloc(sizeof *pi);
        *pi = i;
        append(pi, pl);
    }

    walklist(pl, -1, &checkbackwards, pl);
}
*/

void test5(void)
{
    List *pl = newlist();
    int i;

    fprintf(stderr, "%s\n", __func__);

    for (i = 0; i <= 6; i++) {
        int *pi = malloc(sizeof *pi);
        *pi = i;
        append(pi, pl);
    }

    walklist(pl, &printintptr, NULL);
}

void test6(void)
{
    List *pl = newlist();
    int i;

    fprintf(stderr, "%s\n", __func__);

    for (i = 0; i <= 1000000; i++) {
        int *pi = malloc(sizeof *pi);
        *pi = i;
        append(pi, pl);
    }

    walklist(pl, &donothing, NULL);
}

void test7(void)
{
    List *pl = newlist();

    fprintf(stderr, "%s\n", __func__);

    walklist(pl, &printintptr, NULL);
}

/*
 * custom step support removed from walklist to keep things simple
void test8(void)
{
    List *pl = newlist();
    int i;

    fprintf(stderr, "%s\n", __func__);

    for (i = 0; i <= 9; i++) {
        int *pi = malloc(sizeof *pi);
        *pi = i;
        append(pi, pl);
    }

    walklist(pl, 3, &printintptr, NULL);
}
*/

int getintwidth(void *pvint, void *pvoptions)
{
    int *pi = (int *)pvint;
    /* XXX use asprintf etc. */
    char buf[1024];
    return sprintf(buf, "%d", *pi);
}

int printint(void *pv, void *pvoptions)
{
    int *pi = (int *)pv;

    return printf("%d", *pi);
}

void test9(void)
{
    List *pl = newlist();
    int i;

    fprintf(stderr, "%s\n", __func__);

    for (i = 0; i <= 9; i++) {
        int *pi = malloc(sizeof *pi);
        *pi = i;
        append(pi, pl);
    }

    printlistacross(pl, 10, &getintwidth, &printint, NULL);
}

void test10(void)
{
    List *pl = newlist();
    int i;

    for (i = 0; i <= 9; i++) {
        int *pi = malloc(sizeof *pi);
        *pi = i;
        append(pi, pl);
    }

    printlistdown(pl, 10, &getintwidth, &printint, NULL);
}


/* vim: set ts=4 sw=4 tw=0 et:*/
