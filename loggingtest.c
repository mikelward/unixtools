#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"

void testvalidstring();
void testtoolongstring();
void testerrorffuncname();

int main(int argc, char **argv)
{
    myname = "loggingtest";

    testerrorffuncname();
    testvalidstring();
    testtoolongstring();
    return 0;
}

void testerrorffuncname()
{
    errorf("This is a message from function %s\n", __func__);
}

void testvalidstring()
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

void testtoolongstring()
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
