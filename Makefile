CC=c99
CFLAGS=-Wall -Werror -g

all: ls2

clean:
	-rm *.o

ls2: ls2.o list.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

#  vim: set ts=4 sw=4 tw=0 noet:
