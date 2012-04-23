#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"

int test_bare_file();
int test_absolute_file();

int main(int argc, char **argv)
{
	test_bare_file();
	test_absolute_file();
	return 0;
}

int test_bare_file(void)
{
	File *file = newfile(".", "buf.c");
	assert(strcmp(getname(file), "buf.c") == 0);
	assert(strcmp(getpath(file), "./buf.c") == 0);
	char *dir = getdir(file);
	assert(strcmp(dir, ".") == 0);
	free(dir);
	char *filename = getfile(file);
	assert(strcmp(filename, "buf.c") == 0);
	free(filename);
	free(file);
	return 0;
}

int test_absolute_file(void)
{
	File *file = newfile(".", "/tmp/foo");
	assert(strcmp(getname(file), "/tmp/foo") == 0);
	assert(strcmp(getpath(file), "/tmp/foo") == 0);
	char *dir = getdir(file);
	assert(strcmp(dir, "/tmp") == 0);
	free(dir);
	char *filename = getfile(file);
	assert(strcmp(filename, "foo") == 0);
	free(filename);
	free(file);
	return 0;
}