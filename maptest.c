#define _XOPEN_SOURCE 600

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "map.h"

void test_new_map_is_empty();
void test_map_has_set_item();
void test_item_with_duplicate_hashcode_doesnt_overwrite_existing_item();
void test_map_can_hold_pair_with_null_value();

int main(int argc, char const *argv[])
{
    myname = "maptest";
    
    test_new_map_is_empty();
    test_map_has_set_item();
    test_item_with_duplicate_hashcode_doesnt_overwrite_existing_item();
    test_map_can_hold_pair_with_null_value();
    return 0;
}

void test_new_map_is_empty()
{
    errorf("\n");   /* prints the function name */
    Map *map = newmap();

    int key = 3;

    char *res = get(map, key);

    assert(res == NULL);
}

void test_map_has_set_item()
{
    errorf("\n");   /* prints the function name */
    Map *map = newmap();

    int key = 3;
    char *val = "three";

    set(map, key, val);
    char *res = get(map, key);

    assert(strcmp(res, "three") == 0);
}

void test_item_with_duplicate_hashcode_doesnt_overwrite_existing_item()
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

void test_map_can_hold_pair_with_null_value()
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
