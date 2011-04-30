CC=c99
CFLAGS=-Wall -Werror -g

TESTS=listtest
PROGS=ls2

all: tags $(TESTS) $(PROGS)

tags: *.c
	ctags -R

clean:
	-rm *.o

listtest: listtest.o list.o
	$(CC) $(LDFLAGS) -o $@ $^
	./$@

ls2: ls2.o list.o file.o

#  vim: set ts=4 sw=4 tw=0 noet:
