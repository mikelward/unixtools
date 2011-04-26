CC=c99
CFLAGS=-Wall -Werror -g

all: tags ls2

tags: *.c
	ctags -R

clean:
	-rm *.o

ls2: ls2.o list.o file.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

#  vim: set ts=4 sw=4 tw=0 noet:
