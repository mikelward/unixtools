#ifndef LOGGING_H
#define LOGGING_H

#include <stdarg.h>

char *myname;       /* defined in the main module */
void error(const char *format, ...);
void errorf(const char *func, const char *format, ...);

#endif
/* vim: set ts=4 sw=4 tw=0 et:*/
