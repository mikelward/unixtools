struct list {
    void **data;
    unsigned capacity, next;
};
typedef struct list List;

typedef int (*qsort_compare_function)(const void *a, const void *b);
typedef int (*list_compare_function)(const void **a, const void **b);
typedef void (*walker_func)(void *elem, void *context);
typedef int (*printer_func)(void *elem, void *pvoptions);
typedef int (*width_func)(void *elem, void *pvoptions);

List *newlist(void);
void freelist(List *list);
void append(void *element, List *list);
void *getitem(List *list, unsigned index);
unsigned length(List *list);
void sortlist(List *list, list_compare_function compare);
void walklist(List *list, walker_func func, void *context);
void printlist(List *list, printer_func printer, void *pvoptions);
void printlistacross(List *list,
                     int screenwidth, width_func getwidth,
                     printer_func printer, void *pvoptions);
void printlistdown(List *list,
                   int screenwidth, width_func getwidth,
                   printer_func printer, void *pvoptions);
void printspaces(int n);

/* vim: set ts=4 sw=4 tw=0 et:*/
