#ifndef BUF_H
#define BUF_H

#include <stdbool.h>

enum escape { ESCAPE_NONE, ESCAPE_QUESTION, ESCAPE_C };

typedef struct buf {
    char *data;
    size_t size;
    size_t pos;
    size_t screenpos;
} Buf;

Buf *newbuf(void);
void freebuf(Buf *buf);
void bufappend(Buf *buf, char *string, size_t width, bool printable);
void bufappendchar(Buf *buf, char c);
char *bufstring(Buf *buf);
size_t bufpos(Buf *buf);
size_t bufscreenpos(Buf *buf);
void printtobuf(const char *text, enum escape escape, Buf *buf);

#endif
/* vim: set ts=4 sw=4 tw=0 et:*/
