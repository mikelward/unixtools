#define _XOPEN_SOURCE 600		/* for strdup() */

#include <stdlib.h>
#include <string.h>

#include "logging.h"

typedef struct pair {
	int key;
	char *value;
} Pair;

Pair *newpair(int key, char *value)
{
	Pair *pair = malloc(sizeof(*pair));
	if (!pair) {
		errorf("Out of memory?\n");
		return NULL;
	}

	pair->key = key;
	if (value) {
		pair->value = strdup(value);
		if (!pair->value) {
			errorf("Out of memory?\n");
			free(pair);
			return NULL;
		}
	}
	
	return pair;
}

void freepair(Pair *pair)
{
	if (!pair) return;
	free(pair->value);
	free(pair);
}

int getkey(Pair *pair)
{
	if (!pair) {
		errorf("pair is NULL\n");
		return 0;
	}

	return pair->key;
}

char *getvalue(Pair *pair)
{
	if (!pair) {
		errorf("pair is NULL\n");
		return 0;
	}

	return pair->value;
}

void setvalue(Pair *pair, char *value)
{
	if (!pair) {
		errorf("pair is NULL\n");
		return;
	}

	free(pair->value);
	if (value) {
		pair->value = strdup(value);
		if (!pair->value) {
			errorf("Out of memory?\n");
			return;
		}
	} else {
		pair->value = value;
	}
}