#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "logging.h"
#include "options.h"

char *cescape(char c);

Buf *newbuf(void)
{
    Buf *buf = malloc(sizeof(*buf));
    if (!buf) {
        errorf("buf is NULL\n");
        return NULL;
    }
    buf->size = 1024, buf->data = malloc(1024);
    if (!buf->data) {
        errorf("buf->data is NULL\n");
        free(buf);
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
        if (buf->data) {
            free(buf->data);
        }
        free(buf);
    }
}

void bufappend(Buf *buf, char *string, size_t width, bool printable)
{
    if (!buf) {
        errorf("buf is NULL\n");
        return;
    }
    /* buf->data should never be null since newbuf checks for that */
    assert(buf->data != NULL);
    if (buf->pos + width >= buf->size) {
        /* XXX should this put as much as will fit in the buffer anyway? */
        errorf("string too long\n");
        return;
    }

    strncpy(buf->data+buf->pos, string, width);
    buf->data[buf->pos+width] = '\0';
    buf->pos += width;
    if (printable) {
        buf->screenpos += width;
    }
}

/* c is assumed to be printable */
void bufappendchar(Buf *buf, char c)
{
    if (!buf) {
        errorf("buf is NULL\n");
        return;
    }
    if (buf->pos >= buf->size - 1) {
        errorf("buf is full\n");
        return;
    }

    buf->data[buf->pos] = c;
    buf->pos++; buf->screenpos++;
}

/* return the contents of buf as a NUL terminated string */
char *bufstring(Buf *buf)
{
    if (!buf) {
        errorf("buf is NULL\n");
        return NULL;
    }
    buf->data[buf->pos] = '\0';
    return buf->data;
}

size_t bufpos(Buf *buf)
{
    if (!buf) {
        errorf("buf is NULL\n");
        return 0;
    }
    return buf->pos;
}

size_t bufscreenpos(Buf *buf)
{
    if (!buf) {
        errorf("buf is NULL\n");
        return 0;
    }
    return buf->screenpos;
}

void printtobuf(const char *text, enum escape escape, Buf *buf)
{
    if (!buf) {
        errorf("buf is NULL\n");
        return;
    }
    if (!text) {
        errorf("file is NULL\n");
        return;
    }

    const char *p = text;
    for (p = text; *p != '\0'; p++) {
        if (!isprint(*p)) {
            switch (escape) {
            case ESCAPE_C:
                {
                    char *escaped = cescape(*p);
                    if (escaped == NULL) {
                        errorf("No C escape for %c\n", *p);
                        bufappendchar(buf, *p);
                    } else {
                        bufappend(buf, escaped, strlen(escaped), 1);
                    }
                    break;
                }
            case ESCAPE_QUESTION:
                bufappendchar(buf, '?');
                break;
            default:
                errorf("Unknown escape mode\n");
                /* fall through */
            case ESCAPE_NONE:
                bufappendchar(buf, *p);
                break;
            }
        } else if (*p == '\\' && escape == ESCAPE_C) {
            bufappendchar(buf, '\\');
            bufappendchar(buf, '\\');
        } else {
            bufappendchar(buf, *p);
        }
    }
}

/* XXX does C really not provide this? */
char *cescape(char c)
{
    switch (c) {
    case '\001': return "\\001";
    case '\002': return "\\002";
    case '\003': return "\\003";
    case '\004': return "\\004";
    case '\005': return "\\005";
    case '\006': return "\\006";
    case '\007': return "\\a";
    case '\010': return "\\b";
    case '\011': return "\\t";
    case '\012': return "\\n";
    case '\013': return "\\v";
    case '\014': return "\\f";
    case '\015': return "\\r";
    case '\016': return "\\016";
    case '\017': return "\\017";
    case '\020': return "\\020";
    case '\021': return "\\021";
    case '\022': return "\\022";
    case '\023': return "\\023";
    case '\024': return "\\024";
    case '\025': return "\\025";
    case '\026': return "\\026";
    case '\027': return "\\027";
    case '\030': return "\\030";
    case '\031': return "\\031";
    case '\032': return "\\032";
    case '\033': return "\\033";
    case '\034': return "\\034";
    case '\035': return "\\035";
    case '\036': return "\\036";
    case '\037': return "\\037";
    default: return NULL;
    }
}

/* vim: set ts=4 sw=4 tw=0 et:*/
