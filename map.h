#ifndef MAP_H
#define MAP_H

#include <inttypes.h>
#include <stdbool.h>

#include "list.h"

/**
 * A Map from unsigned to char *.
 */

typedef struct map Map;

Map *newmap();
void freemap(Map *map);

char *get(Map *map, uintmax_t key);
void set(Map *map, uintmax_t key, char *value);
/**
 * Return true if there is an entry with the given key in the map.
 */
bool inmap(Map *map, uintmax_t key);

#endif
