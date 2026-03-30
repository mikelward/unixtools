CC ?= cc
STD=-std=c99
DEBUG=-g
WARNINGS=-Wall -Werror -Wfatal-errors
UNAME := $(shell uname)
ifeq ($(UNAME),Linux)
ACL_CFLAGS ?= -DHAVE_ACL
ACL_LDFLAGS ?= -lacl
CURSES_LDFLAGS ?= -lcurses
else ifeq ($(UNAME),FreeBSD)
ACL_CFLAGS ?= -DHAVE_ACL
ACL_LDFLAGS ?=
CURSES_LDFLAGS ?= -lcurses
else ifeq ($(UNAME),Darwin)
ACL_CFLAGS ?= -DHAVE_ACL
ACL_LDFLAGS ?=
CURSES_LDFLAGS ?= -lcurses
else
CURSES_LDFLAGS ?= -lcurses
endif
CFLAGS=$(STD) $(WARNINGS) $(DEBUG) $(ACL_CFLAGS)
LDFLAGS=$(WARNINGS) $(DEBUG)

DESTDIR=/usr/local
MD2HTML=pandoc -f markdown -t html

SOURCES=*.c *.h
DOCS=README.html
TESTS=buftest filetest filefieldstest listtest loggingtest maptest ltest
PROGS=l

build: $(PROGS) $(TESTS)

clean:
	$(RM) *.o

tags: $(SOURCES)
	$(RM) $@
	ctags -f $@ *.c *.h

doc: $(DOCS)

%.o: %.c options.h
	$(CC) $(CFLAGS) -o $@ -c $<

%: %.o
	$(CC) $(LDFLAGS) -o $@ $^

%.html: %.md
	$(MD2HTML) -o $@ $<

test: $(TESTS)
	@for test in $(TESTS); do \
		echo $$test; \
		./$$test; \
	done

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

all: tags $(TESTS) $(PROGS) $(DOCS)

l: l.o display.o list.o filefields.o file.o field.o buf.o options.o map.o pair.o user.o group.o logging.o $(CURSES_LDFLAGS) $(ACL_LDFLAGS)

buftest: buftest.o buf.o logging.o

filetest: filetest.o file.o map.o pair.o list.o logging.o $(ACL_LDFLAGS)

filefieldstest: filefieldstest.o filefields.o file.o field.o buf.o display.o options.o map.o pair.o list.o user.o group.o logging.o $(CURSES_LDFLAGS) $(ACL_LDFLAGS)

listtest: listtest.o list.o logging.o

loggingtest: loggingtest.o logging.o

ltest:

maptest: map.o list.o pair.o logging.o

#  vim: set ts=4 sw=4 tw=0 noet:
