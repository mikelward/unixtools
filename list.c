#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "logging.h"

List *newlist(void)
{
    List *list = malloc(sizeof *list);
    if (!list) {
        errorf("Out of memory\n");
        return NULL;
    }
    list->capacity = 1024;
    list->next = 0;
    list->data = malloc(list->capacity * sizeof(void *));
    if (!list->data) {
        free(list);
        return NULL;
    }
    return list;
}

void freelist(List *list, walker_func freeelem)
{
    if (!list) return;
    int i = 0;
    for (; i < list->next; i++) {
        void *elem = (list->data)[i];
        if (elem) {
            freeelem(elem);
            elem = NULL;
        }
    }
    free(list->data);
    list->data = NULL;
    free(list);
}

void append(void *element, List *list)
{
    if (!list) return;
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

void *finditem(List *list, void *item1, equal_func isequal)
{
    if (!list) return NULL;
    if (!item1) return NULL;

    for (int i = 0; i < list->next; i++) {
        void *item2 = (list->data)[i];
        if (isequal(item1, item2)) {
            return item2;
        }
    }
    return NULL;
}

void *getitem(List *list, unsigned index)
{
    if (list == NULL || index >= list->next)
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

/* vim: set ts=4 sw=4 tw=0 et:*/
