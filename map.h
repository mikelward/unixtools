#ifndef MAP_H
#define MAP_H

#include "list.h"

typedef struct map Map;

Map *newmap();
void freemap(Map *map, free_func freer);

char *get(Map *map, int key);
void set(Map *map, int key, char *value);

#endif