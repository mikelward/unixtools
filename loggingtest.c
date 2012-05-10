#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"

void test_copystring_with_string_that_fits();
void test_copystring_with_string_thats_too_long();
void test_errorf_output_includes_function_name();

int main(int argc, char **argv)
{
    myname = "loggingtest";

    test_errorf_output_includes_function_name();
    test_copystring_with_string_that_fits();
    test_copystring_with_string_thats_too_long();
    return 0;
}

void test_errorf_output_includes_function_name()
{
    errorf("This is a message from function %s\n", __func__);
}

void test_copystring_with_string_that_fits()
{
    errorf("\n");
    int bufsize = 4;
    char *buf = malloc(bufsize);
    char *str = "abc";
    char *origbuf = buf;
    copystring(str, &buf, &bufsize);
    assert(strcmp(str, origbuf) == 0);
    assert(bufsize == 1);
}

void test_copystring_with_string_thats_too_long()
{
    errorf("\n");
    int bufsize = 4;
    char *buf = malloc(bufsize);
    char *str = "abcd";
    char *origbuf = buf;
    copystring(str, &buf, &bufsize);
    assert(strcmp(origbuf, "abc") == 0);
    assert(bufsize == 0);
}
