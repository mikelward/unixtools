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
# _XOPEN_SOURCE/_POSIX_C_SOURCE clear __BSD_VISIBLE, which hides st_birthtime.
PORTABILITY_CFLAGS ?= -D__BSD_VISIBLE=1
else ifeq ($(UNAME),Darwin)
ACL_CFLAGS ?= -DHAVE_ACL
ACL_LDFLAGS ?=
CURSES_LDFLAGS ?= -lcurses
# _XOPEN_SOURCE/_POSIX_C_SOURCE lower __DARWIN_C_LEVEL, which hides Darwin
# extensions we rely on (st_birthtime, major(), minor()).  _DARWIN_C_SOURCE
# restores them without giving up the POSIX declarations.
PORTABILITY_CFLAGS ?= -D_DARWIN_C_SOURCE
else
CURSES_LDFLAGS ?= -lcurses
endif
CFLAGS=$(STD) $(WARNINGS) $(DEBUG) $(ACL_CFLAGS) $(PORTABILITY_CFLAGS)
LDFLAGS=$(WARNINGS) $(DEBUG)

# PREFIX picks where the files live on the target system; DESTDIR only stages
# that same tree somewhere else, for packaging.  A home-directory install is
# `make install PREFIX=$HOME/.local'.  DESTDIR used to be the prefix here, so
# it now needs PREFIX instead.
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

MD2HTML=pandoc -f markdown -t html

SOURCES=*.c *.h
DOCS=README.html
TESTS=buftest filetest filefieldstest listtest loggingtest maptest ltest installtest session_start_hook_test workflows_test
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
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.html: %.md
	$(MD2HTML) -o $@ $<

# Every test runs, and any failure survives to the aggregate status. A bare
# loop returns only the last command's status, so a failure anywhere but the
# end was reported as success — which is how CI could accept a regression.
test: $(TESTS)
	@status=0; \
	for test in $(TESTS); do \
		echo $$test; \
		MAKE='$(MAKE)' ./$$test || status=1; \
	done; \
	exit $$status

install: $(PROGS)
	@echo install -d $(DESTDIR)$(BINDIR); \
	install -d $(DESTDIR)$(BINDIR); \
	for prog in $(PROGS); do \
		echo install $$prog $(DESTDIR)$(BINDIR); \
		install $$prog $(DESTDIR)$(BINDIR); \
	done

uninstall:
	@for prog in $(PROGS); do \
		echo rm $(DESTDIR)$(BINDIR)/$$prog; \
		rm $(DESTDIR)$(BINDIR)/$$prog; \
	done

all: tags $(TESTS) $(PROGS) $(DOCS)

# Libraries go in LDLIBS rather than in the prerequisite list.  As `-lfoo'
# prerequisites GNU make has to resolve them via .LIBPATTERNS (`lib%.so
# lib%.a'), which finds nothing on macOS, where the system libraries are
# .dylib/.tbd and are not on disk at all since macOS 11.
l: LDLIBS = $(CURSES_LDFLAGS) $(ACL_LDFLAGS)
l: l.o display.o list.o filefields.o file.o field.o buf.o options.o map.o pair.o user.o group.o logging.o

buftest: buftest.o buf.o logging.o

filetest: LDLIBS = $(ACL_LDFLAGS)
filetest: filetest.o file.o map.o pair.o list.o logging.o

filefieldstest: LDLIBS = $(CURSES_LDFLAGS) $(ACL_LDFLAGS)
filefieldstest: filefieldstest.o filefields.o file.o field.o buf.o display.o options.o map.o pair.o list.o user.o group.o logging.o

listtest: listtest.o list.o logging.o

loggingtest: loggingtest.o logging.o

ltest:

installtest:

maptest: map.o list.o pair.o logging.o

#  vim: set ts=4 sw=4 tw=0 noet:
