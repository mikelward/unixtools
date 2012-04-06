#include "logging.h"

#include <stdio.h>

char errorformat[256];

void error(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
    int bufsize = sizeof(errorformat);
    int pos = 0;
    pos += snprintf(errorformat, bufsize, "%s: ", myname);
    snprintf(errorformat+pos, bufsize-pos, "%s", format);
    errorformat[bufsize-1] = '\0';
	vfprintf(stderr, errorformat, ap);
	va_end(ap);
}

void errorf(const char *func, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
    int bufsize = sizeof(errorformat);
    int pos = 0;
    pos += snprintf(errorformat, bufsize, "%s: %s: ", myname, func);
    snprintf(errorformat+pos, bufsize-pos, "%s", format);
    errorformat[bufsize-1] = '\0';
	vfprintf(stderr, errorformat, ap);
	va_end(ap);
}

/* vim: set ts=4 sw=4 tw=0 et:*/
