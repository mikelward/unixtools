CC=cc
CFLAGS=-std=c99 -Wall -Werror -g

TESTS=listtest
PROGS=l
DESTDIR=/usr/local

all: tags $(TESTS) $(PROGS)

tags: *.c
	$(RM) $@
	ctags -f $@ *.c *.h

clean:
	$(RM) *.o

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

l: l.o display.o list.o file.o field.o buf.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -ltermcap

install: $(PROGS)
	@for prog in $(PROGS); do \
		echo install $$prog $(DESTDIR)/bin; \
		install $$prog $(DESTDIR)/bin; \
	done

uninstall: $(PROGS)
	@for prog in $(PROGS); do \
		echo rm $(DESTDIR)/bin/$$prog; \
		rm $(DESTDIR)/bin/$$prog; \
	done

#  vim: set ts=4 sw=4 tw=0 noet:
