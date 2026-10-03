#include "logging.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

char *myname;

void copystring(const char *str, char **pbuf, int *pbufsize)
{
    char *buf = *pbuf;
    int bufsize = *pbufsize;
    int len;
    if (bufsize <= 0) {
        return;
    }
    for (len = 0; len < bufsize && str[len]; len++) {
        buf[len] = str[len];
    }
    if (len < bufsize) {
        buf[len] = '\0';
    } else {
        buf[bufsize-1] = '\0';
    }
    *pbuf = buf+len;
    *pbufsize = bufsize-len;
}

/*
 * Write msg to stream, escaping each byte of anything the current locale
 * does not consider printable as \NNN.  Messages carry file names, which
 * anyone who can create a file chooses; written raw, a name holding a
 * terminal escape sequence would be interpreted by the user's terminal.
 * A trailing newline is the message's own and is written as-is.
 */
static void fputsescaped(const char *msg, FILE *stream)
{
    const char *p = msg;
    const char *end = msg + strlen(msg);
    mbstate_t state;
    memset(&state, 0, sizeof state);
    while (p < end) {
        if (*p == '\n' && p + 1 == end) {
            putc('\n', stream);
            break;
        }
        wchar_t wc;
        size_t bytes = mbrtowc(&wc, p, end - p, &state);
        int printable;
        if (bytes == (size_t)-1 || bytes == (size_t)-2) {
            memset(&state, 0, sizeof state);
            bytes = 1;
            printable = 0;
        } else {
            printable = iswprint(wc);
        }
        if (printable) {
            fwrite(p, 1, bytes, stream);
        } else {
            for (size_t i = 0; i < bytes; i++) {
                fprintf(stream, "\\%03o", (unsigned char)p[i]);
            }
        }
        p += bytes;
    }
}

void errorf2(const char *func, const char *format, ...)
{
    if (myname) {
        fputs(myname, stderr);
        fputs(": ", stderr);
    }
    if (func) {
        fputs(func, stderr);
        fputs(": ", stderr);
    }
    if (!format) {
        return;
    }

    /* format the whole message first so it can be escaped as a unit */
    char small[1024];
    char *msg = small;
    va_list ap;
    va_start(ap, format);
    int len = vsnprintf(small, sizeof small, format, ap);
    va_end(ap);
    if (len < 0) {
        return;
    }
    if ((size_t)len >= sizeof small) {
        char *big = malloc((size_t)len + 1);
        if (big) {
            va_start(ap, format);
            vsnprintf(big, (size_t)len + 1, format, ap);
            va_end(ap);
            msg = big;
        }
        /* out of memory: print the truncated message rather than nothing */
    }
    fputsescaped(msg, stderr);
    if (msg != small) {
        free(msg);
    }
}

/* vim: set ts=4 sw=4 tw=0 et:*/
