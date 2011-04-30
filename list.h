struct list {
    void **data;
    unsigned capacity, next;
};
typedef struct list List;

typedef int (*qsort_compare_function)(const void *a, const void *b);

List *newlist(void);
void freelist(List *list);
void append(void *element, List *list);
void *getitem(List *list, unsigned index);
unsigned length(List *list);
void sortlist(List *list, int (*compare)(const void **a, const void **b));
void walklist(List *list, int step, void (*func)(void *elem, void *context2), void *context);

/* vim: set ts=4 sw=4 tw=0 et:*/
