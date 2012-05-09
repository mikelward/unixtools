#define _XOPEN_SOURCE 600

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "map.h"

void testmapempty();
void testmaponeitem();
void testmapduplicatehash();
void testmapnullkeyisset();

int main(int argc, char const *argv[])
{
    testmapempty();
    testmaponeitem();
    testmapduplicatehash();
    testmapnullkeyisset();
    return 0;
}

void testmapempty()
{
    errorf("\n");   /* prints the function name */
    Map *map = newmap();

    int key = 3;

    char *res = get(map, key);

    assert(res == NULL);
}

void testmaponeitem()
{
    errorf("\n");   /* prints the function name */
    Map *map = newmap();

    int key = 3;
    char *val = "three";

    set(map, key, val);
    char *res = get(map, key);

    assert(strcmp(res, "three") == 0);
}

void testmapduplicatehash()
{
    errorf("\n");   /* prints the function name */
    Map *map = newmap();

    int key;
    char *val;

    key = 3, val = "three";
    set(map, key, val);

    key = 5, val = "five";
    set(map, key, val);

    key = 11, val = "eleven";
    set(map, key, val);

    key = 13, val = "thirteen";
    set(map, key, val);

    key = 3;
    val = get(map, key);
    assert(strcmp(val, "three") == 0);

    key = 5;
    val = get(map, key);
    assert(strcmp(val, "five") == 0);

    key = 11;
    val = get(map, key);
    assert(strcmp(val, "eleven") == 0);

    key = 13;
    val = get(map, key);
    assert(strcmp(val, "thirteen") == 0);
}

void testmapnullkeyisset()
{
    errorf("\n");   /* prints the function name */
    Map *map = newmap();

    int key;
    char *val;

    key = 3, val = "three";
    set(map, key, val);

    key = 5, val = NULL;
    set(map, key, val);

    assert(inmap(map, 3));
    assert(inmap(map, 5));
    assert(!inmap(map, 7));
}
