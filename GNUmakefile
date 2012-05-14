CC=c99
DEBUG=-g
WARNINGS=-Wall -Werror -Wfatal-errors
CFLAGS=$(WARNINGS) $(DEBUG)
LDFLAGS=$(WARNINGS) $(DEBUG)

DESTDIR=/usr/local
MD2HTML=pandoc -f markdown -t html

SOURCES=*.c *.h
DOCS=README.html
TESTS=filetest listtest loggingtest maptest
PROGS=l

build: $(PROGS)

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

l: l.o display.o list.o filefields.o file.o field.o buf.o options.o map.o pair.o user.o group.o logging.o -ltermcap -lacl

filetest: filetest.o file.o logging.o -lacl

listtest: listtest.o list.o logging.o

loggingtest: loggingtest.o logging.o

maptest: map.o list.o pair.o logging.o

#  vim: set ts=4 sw=4 tw=0 noet:
