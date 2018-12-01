#define _XOPEN_SOURCE 600

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "file.h"
#include "logging.h"

int test_bare_file();
int test_absolute_file();
int test_relative_file();
int test_dot();
int test_filename();
int test_fileperms();

int main(int argc, char **argv)
{
    myname = "filetest";

    test_bare_file();
    test_absolute_file();
    test_relative_file();
    test_dot();
    test_filename();
    test_fileperms();
    return 0;
}

int test_bare_file(void)
{
    errorf("\n");   /* prints the function name */
    File *file = newfile(".", "buf.c");
    assert(strcmp(getname(file), "buf.c") == 0);
    assert(strcmp(getpath(file), "./buf.c") == 0);
    char *dir = getdirname(file);
    assert(strcmp(dir, ".") == 0);
    free(dir);
    char *base = getbasename(file);
    assert(strcmp(base, "buf.c") == 0);
    free(base);
    free(file);
    return 0;
}

int test_absolute_file(void)
{
    errorf("\n");   /* prints the function name */
    File *file = newfile(".", "/tmp/foo");
    assert(strcmp(getname(file), "/tmp/foo") == 0);
    assert(strcmp(getpath(file), "/tmp/foo") == 0);
    char *dir = getdirname(file);
    assert(strcmp(dir, "/tmp") == 0);
    free(dir);
    char *base = getbasename(file);
    assert(strcmp(base, "foo") == 0);
    free(base);
    free(file);
    return 0;
}

int test_relative_file(void)
{
    errorf("\n");   /* prints the function name */
    File *file = newfile(".", "../tmp/foo");
    assert(strcmp(getname(file), "../tmp/foo") == 0);
    assert(strcmp(getpath(file), "./../tmp/foo") == 0);
    char *dir = getdirname(file);
    assert(strcmp(dir, "./../tmp") == 0);
    free(dir);
    char *base = getbasename(file);
    assert(strcmp(base, "foo") == 0);
    free(base);
    free(file);
    return 0;
}

int test_dot(void)
{
    errorf("\n");   /* prints the function name */
    File *file = newfile("", ".");
    assert(strcmp(getname(file), ".") == 0);
    assert(strcmp(getpath(file), ".") == 0);
    char *dir = getdirname(file);
    assert(strcmp(dir, ".") == 0);
    free(dir);
    free(file);
    return 0;
}

int test_filename(void)
{
    errorf("\n");   /* prints the function name */
    char tempfilename[L_tmpnam];
    strcpy(tempfilename, "/tmp/filetestXXXXXX");
    int fd = mkstemp(tempfilename);
    assert(fd > 0);

    File *file = newfile("", tempfilename);
    const char *name = getname(file);
    assert(strcmp(tempfilename, name) == 0);
    free(file);
    close(fd);
    unlink(tempfilename);
    return 0;
}

int test_fileperms(void)
{
    errorf("\n");   /* prints the function name */
    char tempfilename[L_tmpnam];
    strcpy(tempfilename, "/tmp/filetestXXXXXX");
    int fd = mkstemp(tempfilename);
    assert(fd > 0);
    assert(chmod(tempfilename, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) == 0);

    File *file = newfile("", tempfilename);
    char *modes = getmodes(file);
    /* space at end assuming no extended ACLs on current directory */
    assert(strcmp(modes, "-rw-r--r-- ") == 0);
    free(modes);

    char *perms = getperms(file);
    assert(strcmp(perms, "rw-") == 0);
    free(perms);
    close(fd);
    unlink(tempfilename);

    return 0;
}
