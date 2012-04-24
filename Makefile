CC=cc
CFLAGS=-std=c99 -Wall -Werror -g

TESTS=filetest listtest
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

filetest.o: filetest.c
	$(CC) $(CFLAGS) -c -o $@ $<

filetest: filetest.o file.o user.o group.o logging.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lacl
	./$@

group.o: group.c
	$(CC) $(CFLAGS) -c -o $@ $<

listtest.o: listtest.c
	$(CC) $(CFLAGS) -c -o $@ $<

list.o: list.c
	$(CC) $(CFLAGS) -c -o $@ $<

logging.o: logging.c
	$(CC) $(CFLAGS) -c -o $@ $<

user.o: user.c
	$(CC) $(CFLAGS) -c -o $@ $<

listtest: listtest.o list.o logging.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	./$@

l: l.o display.o list.o file.o field.o user.o group.o buf.o logging.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -ltermcap -lacl

install: $(PROGS)
	@echo install -d $(DESTDIR)/bin; \
	install -d $(DESTDIR)/bin; \
	for prog in $(PROGS); do \
		echo install $$prog $(DESTDIR)/bin; \
		install $$prog $(DESTDIR)/bin; \
	done

uninstall: $(PROGS)
	@for prog in $(PROGS); do \
		echo rm $(DESTDIR)/bin/$$prog; \
		rm $(DESTDIR)/bin/$$prog; \
	done

#  vim: set ts=4 sw=4 tw=0 noet:
