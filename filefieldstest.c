#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filefields.h"
#include "logging.h"

void test_humanbytes(void);
void test_humanbytes_large(void);
void test_humanbytes_precision(void);

int main(int argc, char **argv)
{
    myname = "filefieldstest";

    test_humanbytes();
    test_humanbytes_large();
    test_humanbytes_precision();
    return 0;
}

void test_humanbytes(void)
{
    errorf("\n");
    char *s;

    s = humanbytes(0);
    assert(strcmp(s, "0 B") == 0);
    free(s);

    s = humanbytes(999);
    assert(strcmp(s, "999 B") == 0);
    free(s);

    s = humanbytes(1000);
    assert(strcmp(s, "1 KB") == 0);
    free(s);

    s = humanbytes(1000000);
    assert(strcmp(s, "1 MB") == 0);
    free(s);

    s = humanbytes(1000000000);
    assert(strcmp(s, "1 GB") == 0);
    free(s);
}

void test_humanbytes_precision(void)
{
    errorf("\n");
    /* 1900 bytes -> 2 KB with rounding and no trailing space in source */
    char *result = humanbytes(1900);
    printf("1900 bytes -> %s\n", result);
    assert(strcmp(result, "2 KB") == 0);
    free(result);
}

void test_humanbytes_large(void)
{
    errorf("\n");
    /* ULONG_MAX would have crashed before the fix */
    char *result = humanbytes(ULONG_MAX);
    assert(result != NULL);
    assert(strlen(result) > 0);
    free(result);
}

/* vim: set ts=4 sw=4 tw=0 et:*/
