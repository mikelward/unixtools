#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE  // for wcwidth

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "buf.h"
#include "logging.h"
#include "options.h"

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
    memset(&buf->shiftstate, 0, sizeof buf->shiftstate);
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

/**
 * Append string to buf.
 *
 * @param buf           A Buf to append to.
 * @param string        The string to append to buf.
 * @param width         The number of bytes in string.
 * @param columns       The number of columns needed to display string.
 */
void bufappend(Buf *buf, char *string, size_t width, size_t columns)
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
    buf->screenpos += columns;
}

/**
 * Append srcbuf to buf.
 */
void bufappendbuf(Buf *buf, Buf *srcbuf)
{
    if (!buf) {
        errorf("buf is NULL\n");
        return;
    }
    if (!srcbuf) {
        errorf("srcbuf is NULL\n");
        return;
    }

    char *string = srcbuf->data;
    size_t width = srcbuf->pos;
    size_t columns = srcbuf->screenpos;
    bufappend(buf, string, width, columns);
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

/* append a wide character as-is, accounting for its display width */
void bufappendwchar(Buf *buf, wchar_t wc)
{
    if (!buf) {
        errorf("buf is NULL\n");
        return;
    }

    char mbbuf[MB_LEN_MAX];
    int columns = wcwidth(wc);
    size_t width = wcrtomb(mbbuf, wc, &buf->shiftstate);
    if (width == -1) {
        // XXX: Append a question mark? Return error and handle in caller?
        errorf("Invalid multibyte sequence\n");
        return;
    }
    bufappend(buf, mbbuf, width, columns);
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

/**
 * Put "text" in buf.
 *
 * Text may be escaped depending on the value of escape and the current locale.
 *
 * @param text          The string to append to buf.
 * @param escape        How to escape non-printable characters.
 * @param buf           The buffer to append to.
 */
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
    wchar_t wc;
    mbstate_t ps = { 0 };
    size_t bytes;
    Buf *bytebuf = newbuf();
    while ((bytes = mbrtowc(&wc, p, MB_LEN_MAX, &ps)) > 0) {
        printwchartobuf(wc, escape, bytebuf);
        p += bytes;
    }
    if (bytes != 0) {
        errorf("Incomplete multibyte character\n");
        return;
    }
    bufappendbuf(buf, bytebuf);
}

/* append a wide character to buf, possibly escaping control chars */
void printwchartobuf(wchar_t wc, enum escape escape, Buf *buf)
{
    if (wc == L'\\' && escape == ESCAPE_C) {
        /* backslash becomes double backslash */
        bufappendchar(buf, '\\');
        bufappendchar(buf, '\\');
    } else if (iswprint(wc)) {
        bufappendwchar(buf, wc);
    } else if (iswcntrl(wc)) {
        switch (escape) {
        case ESCAPE_C:
            printwesctobuf(wc, buf);
            break;
        case ESCAPE_QUESTION:
            bufappendchar(buf, '?');
            break;
        case ESCAPE_NONE:
            bufappendwchar(buf, wc);
            break;
        default:
            errorf("Unknown escape mode\n");
            bufappendwchar(buf, wc);
            break;
        }
    } else {
        errorf("Unknown character type, is locale initialized?\n");
    }
}

/* append a wide character to buf, escaped C-style */
void printwesctobuf(wchar_t wc, Buf *buf)
{
    int byte = wctob(wc);
    if (byte != EOF) {
        char *escaped = cescape(byte);
        if (escaped == NULL) {
            errorf("No C escape for %c\n", byte);
            bufappendchar(buf, byte);
        } else {
            size_t len = strlen(escaped);
            bufappend(buf, escaped, len, len);
        }
    } else {
        errorf("Inescapable multibyte character\n");
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
