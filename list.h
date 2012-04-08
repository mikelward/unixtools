#ifndef LIST_H
#define LIST_H

struct list {
    void **data;
    unsigned capacity, next;
};
typedef struct list List;

typedef int (*qsort_compare_function)(const void *a, const void *b);
typedef int (*list_compare_function)(const void **a, const void **b);
typedef void * (*map_func)(void *elem, void *context);
typedef void (*free_func)(void *elem);
typedef void (*walker_func)(void *elem);
typedef void (*walker_context_func)(void *elem, void *context);
typedef int (*printer_func)(void *elem, void *pvoptions);
typedef int (*width_func)(void *elem, void *pvoptions);

List *newlist(void);
void freelist(List *list, free_func freeelem);

void append(void *element, List *list);
unsigned length(List *list);
void *getitem(List *list, unsigned index);
List *map(List *list, map_func func, void *context);
void setitem(List *list, unsigned index, void *element);
void sortlist(List *list, list_compare_function compare);
void reverselist(List *list);
void walklist(List *list, walker_func func);
void walklistcontext(List *list, walker_context_func func, void *context);
void printlist(List *list, printer_func printer, void *pvoptions);
void printlistacross(List *list,
                     int screenwidth, width_func getwidth,
                     printer_func printer, void *pvoptions);
void printlistdown(List *list,
                   int screenwidth, width_func getwidth,
                   printer_func printer, void *pvoptions);
/* XXX move to display.[ch] */
void printspaces(int n);
int ceildiv(int num, int mult);

#endif
/* vim: set ts=4 sw=4 tw=0 et:*/
