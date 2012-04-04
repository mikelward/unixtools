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

buf.o: buf.c
	$(CC) $(CFLAGS) -c -o $@ $<

display.o: display.c
	$(CC) $(CFLAGS) -c -o $@ $<

field.o: field.c
	$(CC) $(CFLAGS) -c -o $@ $<

listtest.o: listtest.c
	$(CC) $(CFLAGS) -c -o $@ $<

list.o: list.c
	$(CC) $(CFLAGS) -c -o $@ $<

listtest: listtest.o list.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ listtest.o list.o
	./$@

ls2: ls2.o display.o list.o file.o field.o buf.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -ltermcap

#  vim: set ts=4 sw=4 tw=0 noet:
