#ifndef LOGGING_H
#define LOGGING_H

#include <stdarg.h>

extern char *myname;       /* defined in the main module */

/**
 * Marks a function as taking a printf() format string and its arguments, so
 * that the compiler checks each call's arguments against its format.
 *
 * fmt and first are the positions of the format string and of the first
 * argument it formats, counting from 1.
 */
#ifdef __GNUC__
#define PRINTFLIKE(fmt, first) __attribute__((__format__(__printf__, fmt, first)))
#else
#define PRINTFLIKE(fmt, first)
#endif

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
void errorf2(const char *func, const char *format, ...) PRINTFLIKE(2, 3);

#endif
/* vim: set ts=4 sw=4 tw=0 et:*/
