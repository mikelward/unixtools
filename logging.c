#include "logging.h"

#include <stdio.h>

char errorformat[256];

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
    *pbuf = buf;
    *pbufsize = bufsize-len;
}

void errorf2(const char *func, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
    int bufsize = sizeof(errorformat);
    char *buf = errorformat;
    if (myname) {
        copystring(myname, &buf, &bufsize);
        copystring(": ", &buf, &bufsize);
    }
    if (func) {
        copystring(func, &buf, &bufsize);
        copystring(": ", &buf, &bufsize);
    }
    if (format) {
        copystring(format, &buf, &bufsize);
    }
    if (bufsize <= 0) {
        errorf("Log message truncated to %d bytes\n", sizeof(errorformat));
    }
	vfprintf(stderr, errorformat, ap);
	va_end(ap);
}

/* vim: set ts=4 sw=4 tw=0 et:*/
