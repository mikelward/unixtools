#include "logging.h"

#include <stdarg.h>
#include <stdio.h>

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

void errorf2(const char *func, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    if (myname) {
        fputs(myname, stderr);
        fputs(": ", stderr);
    }
    if (func) {
        fputs(func, stderr);
        fputs(": ", stderr);
    }
    if (format) {
        vfprintf(stderr, format, ap);
    }
    va_end(ap);
}

/* vim: set ts=4 sw=4 tw=0 et:*/
