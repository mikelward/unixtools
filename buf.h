#ifndef BUF_H
#define BUF_H

#include <stdbool.h>

typedef struct buf {
	char *data;
	int size;
	int pos;
	int screenpos;
} Buf;

Buf *newbuf(void);
void freebuf(Buf *buf);	
void bufappend(Buf *buf, char *string, int width, bool printable);
void bufappendchar(Buf *buf, char c);
char *bufstring(Buf *buf);
int bufpos(Buf *buf);
int bufscreenpos(Buf *buf);

#endif
/* vim: set ts=4 sw=4 tw=0 et:*/
