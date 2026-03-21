#include <assert.h>
#include <limits.h>
#include <stdio.h>
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
    assert(strcmp(humanbytes(0), "0 B") == 0);
    assert(strcmp(humanbytes(999), "999 B") == 0);
    assert(strcmp(humanbytes(1000), "1 KB") == 0);
    assert(strcmp(humanbytes(1000000), "1 MB") == 0);
    assert(strcmp(humanbytes(1000000000), "1 GB") == 0);
}

void test_humanbytes_precision(void)
{
    errorf("\n");
    /* 1900 bytes -> 2 KB with rounding and no trailing space in source */
    const char *result = humanbytes(1900);
    printf("1900 bytes -> %s\n", result);
    assert(strcmp(result, "2 KB") == 0);
}

void test_humanbytes_large(void)
{
    errorf("\n");
    /* ULONG_MAX would have crashed before the fix */
    const char *result = humanbytes(ULONG_MAX);
    assert(result != NULL);
    assert(strlen(result) > 0);
}

/* vim: set ts=4 sw=4 tw=0 et:*/
