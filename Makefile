CC=cc
CFLAGS=-std=c99 -Wall -Werror -g

TESTS=listtest
PROGS=ls2

all: tags $(TESTS) $(PROGS)

tags: *.c
	-rm $@
	ctags -f $@ *.c *.h

clean:
	-rm *.o

listtest.o: listtest.c
	$(CC) $(CFLAGS) -c -o $@ $<

list.o: list.c
	$(CC) $(CFLAGS) -c -o $@ $<

listtest: listtest.o list.o
	$(CC) $(LDFLAGS) -o $@ listtest.o list.o
	./$@

ls2: ls2.o list.o file.o
	$(CC) $(LDFLAGS) -o $@ ls2.o list.o file.o -ltermcap

#  vim: set ts=4 sw=4 tw=0 noet:
