
#include "map.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test1();

int main(int argc, char const *argv[])
{
	test1();
	return 0;
}

void test1()
{
	Map *map = newmap();

	int key = 3;
	char *val = "three";

	set(map, key, val);
	char *res = get(map, key);

	assert(strcmp(res, "three") == 0);
}

void test2()
{
	Map *map = newmap();

	int key = 3;

	char *res = get(map, key);

	assert(res == NULL);
}
