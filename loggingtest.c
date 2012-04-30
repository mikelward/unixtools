#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"

void test1();
void test2();
void test3();

int main(int argc, char **argv)
{
    myname = "loggingtest";

    test1();
    test2();
    test3();
    return 0;
}

void test1()
{
    int bufsize = 4;
    char *buf = malloc(bufsize);
    char *str = "abc";
    char *origbuf = buf;
    copystring(str, &buf, &bufsize);
    assert(strcmp(str, origbuf) == 0);
    assert(bufsize == 1);
}

void test2()
{
    int bufsize = 4;
    char *buf = malloc(bufsize);
    char *str = "abcd";
    char *origbuf = buf;
    copystring(str, &buf, &bufsize);
    assert(strcmp(origbuf, "abc") == 0);
    assert(bufsize == 0);
}

void test3()
{
    errorf("This is a message from function %s\n", __func__);
}
