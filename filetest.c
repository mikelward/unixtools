#define _XOPEN_SOURCE 600

#include <sys/types.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/sysmacros.h>  /* for major(), minor() - on BSDs these are in <sys/types.h> */
#endif
#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
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
int test_device_numbers();
int test_blocks();

int main(int argc, char **argv)
{
    myname = "filetest";

    test_bare_file();
    test_absolute_file();
    test_relative_file();
    test_dot();
    test_filename();
    test_fileperms();
    test_device_numbers();
    test_blocks();
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

int test_device_numbers(void)
{
    errorf("\n");   /* prints the function name */

    /* Device numbers are platform-specific: /dev/null is 1,3 on Linux and
       3,2 on Darwin. Comparing against what stat() reports here tests what
       getmajor()/getminor() are actually for — decoding st_rdev — without
       baking one platform's numbering into the assertion. */
    struct stat st;

    assert(stat("/dev/null", &st) == 0);
    File *file = newfile("", "/dev/null");
    assert(isstat(file));
    assert(ischardev(file));
    assert(isdevice(file));
    assert(getmajor(file) == major(st.st_rdev));
    assert(getminor(file) == minor(st.st_rdev));
    freefile(file);

    assert(stat("/dev/zero", &st) == 0);
    file = newfile("", "/dev/zero");
    assert(isstat(file));
    assert(ischardev(file));
    assert(isdevice(file));
    assert(getmajor(file) == major(st.st_rdev));
    assert(getminor(file) == minor(st.st_rdev));
    freefile(file);

    /* a regular file is not a device */
    file = newfile(".", "file.c");
    assert(isstat(file));
    assert(!isdevice(file));
    freefile(file);

    return 0;
}

/* a block size that 512 neither divides nor is divided by used to be
   rounded to one that does, so 1000 counted 512-byte blocks */
int test_blocks(void)
{
    errorf("\n");   /* prints the function name */
    char name[] = "filetest.XXXXXX";
    int fd = mkstemp(name);
    assert(fd != -1);
    char data[10000];
    memset(data, 'x', sizeof data);
    assert(write(fd, data, sizeof data) == (ssize_t)sizeof data);
    /* so that the file's allocation is settled before it is stat'd */
    assert(fsync(fd) == 0);
    assert(close(fd) == 0);

    struct stat st;
    assert(stat(name, &st) == 0);
    uintmax_t bytes = (uintmax_t)st.st_blocks * 512;
    File *file = newfile(".", name);
    unsigned blocksizes[] = { 1, 100, 512, 1000, 1024, 2000, 4096, 1000000 };
    for (size_t i = 0; i < sizeof blocksizes / sizeof blocksizes[0]; i++) {
        unsigned blocksize = blocksizes[i];
        assert(getblocks(file, blocksize) == (bytes + blocksize / 2) / blocksize);
    }
    freefile(file);
    assert(unlink(name) == 0);

    return 0;
}
