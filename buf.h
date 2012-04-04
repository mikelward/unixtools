#ifndef BUF_H
#define BUF_H

typedef struct buf {
	char *data;
	int size;
	int pos;
	int screenpos;
} Buf;

Buf *newbuf(void);
void freebuf(Buf *buf);	
void bufappend(Buf *buf, char *string, int width, int screenwidth);
char *bufdata(Buf *buf);
int bufpos(Buf *buf);
int bufscreenpos(Buf *buf);

#endif
/* vim: set ts=4 sw=4 tw=0 et:*/
