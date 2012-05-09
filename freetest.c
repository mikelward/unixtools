#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct foo {
    char *buf;
};

struct foo *newfoo()
{
    struct foo *foo = malloc(sizeof(*foo));
    foo->buf = malloc(1024);
    return foo;
}

void nullfunc(struct foo *foo)
{
}

int main(int argc, const char *argv[])
{
    struct foo *foo = newfoo();

    nullfunc(foo);

    free(foo->buf);

    return 0;
}
/* vim: set ts=4 sw=4 tw=0 et:*/
