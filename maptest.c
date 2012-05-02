#define _XOPEN_SOURCE 600

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "map.h"

void test1();
void test2();
void test3();
void test4();

int main(int argc, char const *argv[])
{
    test1();
    test2();
    test3();
    test4();
    return 0;
}

void test1()
{
    errorf("%s", "Running\n");
    Map *map = newmap();

    int key = 3;
    char *val = "three";

    set(map, key, val);
    char *res = get(map, key);

    assert(strcmp(res, "three") == 0);
}

void test2()
{
    errorf("Running\n");
    Map *map = newmap();

    int key = 3;

    char *res = get(map, key);

    assert(res == NULL);
}

void test3()
{
    errorf("Running\n");
    Map *map = newmap();

    int key;
    char *val;

    key = 3, val = "three";
    set(map, key, val);

    key = 13, val = "thirteen";
    set(map, key, val);

    key = 5, val = "five";
    set(map, key, val);

    key = 3;
    val = get(map, key);
    printf("%d => %s\n", key, val);
    assert(strcmp(val, "three") == 0);

    key = 5;
    val = get(map, key);
    printf("%d => %s\n", key, val);
    assert(strcmp(val, "five") == 0);

    key = 13;
    val = get(map, key);
    printf("%d => %s\n", key, val);
    assert(strcmp(val, "thirteen") == 0);
}

void test4()
{
    errorf("Running\n");
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

