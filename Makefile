CC=c99
CFLAGS=-Wall -Werror -g

all: tags ls2 ls3

tags: *.c
	ctags -R

clean:
	-rm *.o

ls2: ls2.o list.o malloc.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

ls3: ls3.o file.o list.o malloc.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

#  vim: set ts=4 sw=4 tw=0 noet:
