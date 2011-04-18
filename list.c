#include <stdio.h>
#include <stdlib.h>

#include "list.h"

List *newlist(void)
{
    List *lp = malloc(sizeof *lp);
    if (!lp) {
        fprintf(stderr, "newlist: Out of memory\n");
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

void freelist(List *list)
{
    for (int i = 0; i < list->next; i++) {
        void *elem = (list->data)[i];
        free(elem);
        elem = NULL;
    }
}

void append(void *element, List *list)
{
    if (!list) return;
    if (list->next == list->capacity) {
        void **newdata = realloc(list->data, (list->capacity+=1024)*sizeof(*newdata));
        if (!newdata) {
            free(list->data);
            fprintf(stderr, "append: Out of memory\n");
            /* exit? */
        }
        list->data = newdata;
    }
    (list->data)[list->next++] = element;
}

unsigned length(List *list)
{
    return list->next;
}

void *getitem(List *list, unsigned index)
{
    return (list->data)[index];
}

void sortlist(List *list, int (*compare)(const void *a, const void *b))
{
    qsort(list->data, list->next, sizeof(void *), compare);
}

/* vim: set ts=4 sw=4 tw=0 et:*/
