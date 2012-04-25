#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "logging.h"

List *newlist(void)
{
    List *lp = malloc(sizeof *lp);
    if (!lp) {
        errorf("Out of memory\n");
        return NULL;
    }
    lp->capacity = 1024;
    lp->next = 0;
    lp->data = malloc(lp->capacity * sizeof(void *));
    if (!lp->data) {
        free(lp);
        return NULL;
    }
    return lp;
}

void freelist(List *list, walker_func freeelem)
{
    for (int i = 0; i < list->next; i++) {
        void *elem = (list->data)[i];
        if (elem != NULL) {
            freeelem(elem);
            elem = NULL;
        }
    }
    free(list);
}

void append(void *element, List *list)
{
    if (list == NULL) return;
    if (list->next == list->capacity) {
        void **newdata = realloc(list->data, (list->capacity+=1024)*sizeof(*newdata));
        if (!newdata) {
            free(list->data);
            errorf("Out of memory\n");
            /* exit? */
        }
        list->data = newdata;
    }
    (list->data)[list->next++] = element;
}

unsigned length(List *list)
{
    if (list == NULL) {
        errorf("list is NULL\n");
        return 0;
    }

    return list->next;
}

void *getitem(List *list, unsigned index)
{
    if (list == NULL || list->next < index)
        return NULL;

    return (list->data)[index];
}

void setitem(List *list, unsigned index, void *elem)
{
    if (list == NULL || list->next < index)
        return;

    (list->data)[index] = elem;
}

List *map(List *list, map_func func, void *context)
{
    if (list == NULL) {
        errorf("list is NULL\n");
        return NULL;
    }
    List *resultlist = newlist();
    if (resultlist == NULL) {
        errorf("result is NULL\n");
        return NULL;
    }

    int len = length(list);
    for (int i = 0; i < len; i++) {
        void *result = (*func)((list->data)[i], context);
        append(result, resultlist);
    }
    return resultlist;
}

void sortlist(List *list, list_compare_function compare)
{
    if (list == NULL || length(list) < 1)
        return;

    qsort(list->data, list->next, sizeof(void *), (qsort_compare_function)compare);
}

void reverselist(List *list)
{
    if (list == NULL || length(list) < 1)
        return;

    int l = length(list);
    int m = l / 2;
    int last = l - 1;
    for (int i = 0; i <= m; i++) {
        void *t = getitem(list, i);
        int j = last - i;
        setitem(list, i, getitem(list, j));
        setitem(list, j, t);
    }
}

void walklistcontext(List *list, walker_context_func func, void *context)
{
    if (list == NULL) {
        errorf("list is NULL\n");
        return;
    }

    int len = length(list);
    for (int i = 0; i < len; i++) {
        (*func)((list->data)[i], context);
    }
}

void walklist(List *list, walker_func func)
{
    if (list == NULL) {
        errorf("list is NULL\n");
        return;
    }

    int len = length(list);
    for (int i = 0; i < len; i++) {
        (*func)((list->data)[i]);
    }
}

void printlist(List *list, printer_func printer, void *pvoptions)
{
    /* is casting function returning int to function returning void valid? */
    walklistcontext(list, (walker_context_func)printer, pvoptions);
}

struct getmaxwidth_context {
    int maxsofar;
    int (*getwidth)(void *elem, void *pvoptions);
    void *pvoptions;
};

void getmaxwidth2(void *elem, void *voidcontext)
{
    struct getmaxwidth_context *pcontext = (struct getmaxwidth_context *)voidcontext;
    int width = (*pcontext->getwidth)(elem, pcontext->pvoptions);
    if (width > pcontext->maxsofar)
        pcontext->maxsofar = width;
}

int getmaxwidth(List *list, int (*getwidth)(void *elem, void *pvoptions), void *pvoptions)
{
    struct getmaxwidth_context context;
    context.maxsofar = 0;
    context.getwidth = getwidth;
    context.pvoptions = pvoptions;
    walklistcontext(list, &getmaxwidth2, &context);

    return context.maxsofar;
}

/**
 * Print a list in rows across the page, e.g.
 * 0  1  2
 * 3  4  5
 *
 * How many columns to use is calculated by this function based on the screenwidth,
 * and the width of each element, as determined by the "getwidth" function.
 *
 * Each element is then printed by calling "printer" on elements in the correct
 * order to produce columns.
 *
 * printer must return the number of characters printed (excluding characters
 * that don't move the cursor, e.g. color escape sequences) so that this function
 * knows how many spaces to print to produce properly aligned columns.
 */
void printlistacross(List *list,
                     int screenwidth, int (*getwidth)(void *elem, void *pvoptions),
                     int (*printer)(void *elem, void *pvoptions), void *pvoptions)
{
    int maxelemwidth = getmaxwidth(list, getwidth, pvoptions);

    /* XXX I prefer using two so that -x and -xF have the same column layout,
     * this also makes the -F and -O options easier to handle
     * but this doesn't match BSD ls, and probably shouldn't be hard-coded,
     * so think about how to make this better */
    int colwidth = maxelemwidth + 2;   /* 2 space margin between columns */
    int len = length(list);
    int cols = screenwidth / colwidth;
    cols = (cols) ? cols : 1;
    int col = 0;
    int i = 0;
    while (i < len) {
        int nchars = printer(getitem(list, i), pvoptions);
        i++;
        col++;
        if (col == cols) {
            printf("\n");
            col = 0;
        }
        else {
            printspaces(colwidth - nchars);
        }
    }
    if (col != 0) {
        printf("\n");
    }
}

/*
 * divide num by mult
 * and return the nearest integer >= the result
 */
int ceildiv(int num, int mult)
{
    if (mult == 0) {
        errorf("division by zero (%d, %d)\n", num, mult);
        assert(mult != 0);
    }
    int res = (num + mult - 1) / mult;
    return res;
}

void printspaces(int n)
{
    for (int i = 0; i < n; i++) {
        putchar(' ');
    }
}

/**
 * Print a list in columns down the page, e.g.
 * 0  2  4
 * 1  3  5
 *
 * How many columns to use is calculated by this function based on the screenwidth,
 * and the width of each element, as determined by the "getwidth" function.
 *
 * Each element is then printed by calling "printer" on elements in the correct
 * order to produce columns.
 *
 * printer must return the number of characters printed (excluding characters
 * that don't move the cursor, e.g. color escape sequences) so that this function
 * knows how many spaces to print to produce properly aligned columns.
 */
void printlistdown(List *list,
                   int screenwidth, int (*getwidth)(void *elem, void *pvoptions),
                   int (*printer)(void *elem, void *pvoptions), void *pvoptions)
{
    if (list == NULL) {
        errorf("list is NULL\n");
        return;
    }

    int maxelemwidth = getmaxwidth(list, getwidth, pvoptions);
    /* XXX I prefer using two so that -x and -xF have the same column layout,
     * this also makes the -F and -O options easier to handle
     * but this doesn't match BSD ls, and probably shouldn't be hard-coded,
     * so think about how to make this better */
    int colwidth = maxelemwidth + 2;   /* 2 space margin between columns */

    int len = length(list);
    if (len <= 0) return;       /* avoid div by zero */
    int maxcols = screenwidth / colwidth;
    maxcols = (maxcols) ? maxcols : 1;
    int rows = ceildiv(len, maxcols);
    int cols = ceildiv(len, rows);

    int col = 0;
    for (int row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            int idx = col*rows + row;
            if (idx >= len) {
                break;
            }
            void *elem = getitem(list, idx);
            int nchars = printer(elem, pvoptions);
            if (col == cols-1) {
                break;
            }
            else {
                printspaces(colwidth-nchars);
            }
        }
        printf("\n");
    }
}

/* vim: set ts=4 sw=4 tw=0 et:*/
