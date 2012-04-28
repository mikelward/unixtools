#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"

void test1();
void test2();

int main(int argc, char **argv)
{
    test1();
    test2();
    //test3();
    return 0;
}

void test1()
{
    char *buf = malloc(4);
    int bufsize = 4;
    char *str = "abc";
    char **pbuf = &buf;
    copystring(str, pbuf, &bufsize);
    assert(strcmp(str, buf) == 0);
    assert(bufsize == 1);
}

void test2()
{
    char *buf = malloc(4);
    int bufsize = 4;
    char *str = "abcd";
    char **pbuf = &buf;
    copystring(str, pbuf, &bufsize);
    assert(strcmp(buf, "abc") == 0);
    assert(bufsize == 0);
}
