struct list {
    void **data;
    unsigned capacity, next;
};
typedef struct list List;

List *newlist(void);
void freelist(List *list);
void append(void *element, List *list);
void *getitem(List *list, unsigned index);
unsigned length(List *list);
void sort(List *list, int (*compare)(const void *a, const void *b));

/* vim: set ts=4 sw=4 tw=0 et:*/