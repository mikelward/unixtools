#ifndef LOGGING_H
#define LOGGING_H

#include <stdarg.h>

char *myname;       /* defined in the main module */

/**
 * Copy str into *pbuf.
 *
 * *pbuf is modified to point to the terminating NUL byte, so
 * copystring can be called repeatedly using the same *pbuf
 * variable.
 *
 * *pbufsize is also modified for the same reason.
 *
 * If *pbufsize == 0 after the call, then *pbuf was not large
 * enough to hold all of str.
 */
void copystring(const char *str, char **pbuf, int *pbufsize);

/**
 * Print an error message.
 *
 * Arguments are the same as printf().
 *
 * @see printf().
 */
#define error(...) errorf2(NULL, __VA_ARGS__)

/**
 * Print an error message prefixed by the name of the function that called it.
 *
 * Arguments are the same as printf().
 *
 * @see printf().
 */
#define errorf(...) errorf2(__func__, __VA_ARGS__)

/**
 * Print an error message, prefixed by the first argument.
 *
 * Remaining arguments are as for printf().
 *
 * @see printf().
 */
void errorf2(const char *func, const char *format, ...);

#endif
/* vim: set ts=4 sw=4 tw=0 et:*/
