CC=cc
CFLAGS=-std=c99 -Wall -Werror -g

SOURCES=*.c *.h
TESTS=filetest listtest
PROGS=l
DOCS=README.html
DESTDIR=/usr/local
MD2HTML=pandoc -f markdown -t html

build: $(PROGS)

doc: $(DOCS)

test: $(TESTS)
	@for test in $(TESTS); do \
		echo $$test; \
		./$$test; \
	done

all: tags $(TESTS) $(PROGS) $(DOCS)

tags: $(SOURCES)
	$(RM) $@
	ctags -f $@ *.c *.h

clean:
	$(RM) *.o

filetest: filetest.o file.o user.o group.o logging.o -lacl

listtest: listtest.o list.o logging.o

l: l.o display.o list.o file.o field.o user.o group.o buf.o logging.o -ltermcap -lacl

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

%.html: %.md
	$(MD2HTML) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%: %.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

#  vim: set ts=4 sw=4 tw=0 noet:
