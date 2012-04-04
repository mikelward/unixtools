#include "buf.h"

#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Buf *newbuf(void)
{
    Buf *buf = malloc(sizeof(buf));
    if (buf == NULL) {
        fprintf(stderr, "newbuf: buf is NULL\n");
        return NULL;
    }
    buf->size = 1024, buf->data = malloc(1024);
    if (buf->data == NULL) {
        fprintf(stderr, "newbuf: buf->data is NULL\n");
        return NULL;
    }
    buf->data[0] = '\0';
    buf->pos = 0;
    buf->screenpos = 0;
    return buf;
}

void freebuf(Buf *buf)
{
    if (buf) {
        if (buf->data)
            free(buf->data);
        free(buf);
    }
}

void bufappend(Buf *buf, char *string, int width, int screenwidth)
{
    if (buf == NULL) {
        fprintf(stderr, "bufappend: buf is NULL\n");
        return;
    }
    /* buf->data should never be null since newbuf checks for that */
    assert(buf->data != NULL);
    if (width + buf->pos > buf->size) {
        fprintf(stderr, "bufappend: string too long\n");
        return;
    }

    strncpy(buf->data+buf->pos, string, width);
    buf->data[buf->pos+width] = '\0';
    buf->pos += width;
    buf->screenpos += screenwidth;
}

char *bufdata(Buf *buf)
{
    if (buf == NULL) {
        fprintf(stderr, "bufdata: buf is NULL\n");
        return NULL;
    }
    return buf->data;
}

int bufpos(Buf *buf)
{
    if (buf == NULL) {
        fprintf(stderr, "bufpos: buf is NULL\n");
        return 0;
    }
    return buf->pos;
}

int bufscreenpos(Buf *buf)
{
    if (buf == NULL) {
        fprintf(stderr, "bufscreenpos: buf is NULL\n");
        return 0;
    }
    return buf->screenpos;
}

/* vim: set ts=4 sw=4 tw=0 et:*/
