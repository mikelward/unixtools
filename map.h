#ifndef MAP_H
#define MAP_H

#include "list.h"

/**
 * A Map from int to char *.
 */

typedef struct map Map;

Map *newmap();
void freemap(Map *map);

char *get(Map *map, int key);
void set(Map *map, int key, char *value);

#endif