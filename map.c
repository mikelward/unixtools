#include <stdlib.h>

#include "list.h"
#include "logging.h"
#include "map.h"
#include "pair.h"

typedef List PairList;

typedef struct map {
	PairList **slots;
	int size;
	int load;
} Map;

int gethashcode(Map *map, int key)
{
	return key % map->size;
}

bool keysequal(Pair *pair1, Pair *pair2)
{
	if (!pair1 || !pair2) return false;
	return getkey(pair1) == getkey(pair2);
}

Pair *findpair(Map *map, int key)
{
	if (!map) {
		errorf("map is NULL\n");
		return NULL;
	}

	int hashcode = gethashcode(map, key);
	PairList *pairs = (map->slots)[hashcode];
	Pair *temppair = newpair(key, NULL);
	if (!temppair) {
		errorf("Out of memory?\n");
		return NULL;
	}
	Pair *foundpair = finditem(pairs, temppair, (equal_func)keysequal);
	free(temppair);
	return foundpair;
}

Map *newmap()
{
	Map *map = malloc(sizeof *map);
	if (!map) {
		errorf("Out of memory?\n");
		return NULL;
	}

	map->size = 10;
	map->slots = calloc(map->size, sizeof(*map->slots));
	for (int i = 0; i < map->size; i++) {
		PairList *list = newlist();
		if (!list) {
			errorf("Out of memory?\n");
			/* XXX assumes calloc makes these NULL by default */
			freemap(map, free);
			return NULL;
		}
		(map->slots)[i] = list;
	}
	map->load = 0;

	return map;
}

void freemap(Map *map, free_func freer)
{
	if (!map) return;

	for (int i = 0; i < map->size; i++) {
		freelist((map->slots)[i], freer);
	}
	free(map->slots);
	free(map);
}

char *get(Map *map, int key)
{
	if (!map) {
		errorf("map is NULL\n");
		return NULL;
	}
	Pair *pair = findpair(map, key);
	if (!pair) return NULL;
	return getvalue(pair);
}

void set(Map *map, int key, char *value)
{
	int hashcode = gethashcode(map, key);
	PairList *pairs = (map->slots)[hashcode];
	/* TODO allocate list here instead? */
	if (!pairs) {
		errorf("Slots not initialized properly\n");
		return;
	}
	/* finditem requires a pair, not a key, so allocate one now... */
	Pair *temppair = newpair(key, value);
	Pair *pair = finditem(pairs, temppair, (equal_func)keysequal);
	if (pair) {
		freepair(temppair);
		setvalue(pair, value);
	} else {
		setvalue(temppair, value);
		append(temppair, pairs);
		map->load++;
	}
}
