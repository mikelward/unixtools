#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "logging.h"

void test_list_appended_items_match_inserted(void);
void test_sortlist(void);
void test_walklist(void);
void test_reverselist(void);
void test_finditem(void);

int main(int argc, const char *argv[])
{
    myname = "listtest";

    test_list_appended_items_match_inserted();
    test_sortlist();
    test_walklist();
    test_reverselist();
    test_finditem();

    return 0;
}

void test_list_appended_items_match_inserted(void)
{
    int i;
    List *pl = newlist();

    errorf("\n");

    for (i = 0; i < 10; i++) {
        int *pi = malloc(sizeof(int));
        *pi = i;
        append(pi, pl);
    }

    for (i = 0; i < length(pl); i++) {
        int *pi = (int *)getitem(pl, i);
        assert(*pi == i);
    }
    freelist(pl, free);
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

void test_sortlist(void)
{
    List *pl = newlist();
    int is[] = { 3, 1, 4, 1, 5, 2, 9 };
    int i;
    size_t nelems = sizeof(is) / sizeof(is[0]);

    errorf("\n");

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
    freelist(pl, free);
}

void sum(void *pnumber, void *context)
{
    int *ptotal = (int *)context;
    int value = *(int *)pnumber;

    *ptotal += value;
}

void test_walklist(void)
{
    List *pl = newlist();
    int i;
    int total = 0;

    errorf("\n");

    for (i = 1; i <= 3; i++) {
        int *pi = malloc(sizeof *pi);
        *pi = i;
        append(pi, pl);
    }

    walklistcontext(pl, &sum, &total);
    //printf("total = %d\n", total);
    assert(total == 6);
    freelist(pl, free);
}

void printintptr(void *elem)
{
    int *pi = (int *)elem;
    printf("%d\n", *pi);
}

void donothing(void *elem)
{
}

/*
 * Return true if the list is in descending order.
 *
 * @param vpthisint     Pointer to an integer element (as void *)
 * @param vpplastint    Pointer to pointer to previous integer element
 *                      (NULL on first pass)
 */
void checkbackwards(void *vpthisint, void *vpplastint /* really void ** */)
{
    if (vpthisint == NULL || vpplastint == NULL)
        return;

    int *pthisint = (int *)vpthisint;
    int thisint = *pthisint;

    int **pplastint = (int **)vpplastint;
    int *plastint = *pplastint;

    if (plastint != NULL) {
        /* we have a previous int */
        int lastint = *plastint;

        assert(thisint <= lastint);
    }

    *pplastint = pthisint;
}

void test_reverselist(void)
{
    List *pl = newlist();
    int i;

    errorf("\n");

    for (i = 1; i <= 3; i++) {
        int *pi = malloc(sizeof *pi);
        *pi = i;
        append(pi, pl);
    }

    reverselist(pl);

    int *pi = &i;
    walklistcontext(pl, &checkbackwards, &pi);
    freelist(pl, free);
}

/*
 * custom step support removed from walklist to keep things simple
void test8(void)
{
    List *pl = newlist();
    int i;

    errorf("\n");

    for (i = 0; i <= 9; i++) {
        int *pi = malloc(sizeof *pi);
        *pi = i;
        append(pi, pl);
    }

    walklist(pl, 3, &printintptr);
}
*/

int getintwidth(void *pvint, void *pvoptions)
{
    int *pi = (int *)pvint;
    char buf[1024];
    return snprintf(buf, 1024, "%d", *pi);
}

int printint(void *pv, void *pvoptions)
{
    int *pi = (int *)pv;

    return printf("%d", *pi);
}

bool intptrsequal(int *pi1, int *pi2)
{
    if (!pi1 || !pi2) return false;
    return *pi1 == *pi2;
}

void test_finditem(void)
{
    List *list = newlist();
    int i;

    errorf("\n");

    for (i = 0; i <= 9; i++) {
        int *pi = malloc(sizeof *pi);
        *pi = i;
        append(pi, list);
    }

    int *pi, *pifound;
    pi = malloc(sizeof *pi);
    *pi = 5;
    pifound = finditem(list, pi, (equal_func)intptrsequal);
    assert(*pifound == 5);

    *pi = 11;
    pifound = finditem(list, pi, (equal_func)intptrsequal);
    assert(!pifound);

    free(pi);
    freelist(list, free);
}

/* vim: set ts=4 sw=4 tw=0 et:*/
