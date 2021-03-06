#ifndef DISPLAY_H
#define DISPLAY_H

#include "list.h"

/* terminal escape sequences for -G and -K flags
 * not currently customizable
 * see terminfo(3) for details */
typedef struct colors {
    char *black;
    char *red;
    char *green;
    char *yellow;
    char *blue;
    char *magenta;
    char *cyan;
    char *white;
    char *none;                     /* escape sequence to go back to default color */
} Colors;

typedef List StringList;

void printacross(StringList *list, int stringwidth, int screenwidth);
void printdown(StringList *list, int stringwidth, int screenwidth);
void printspaces(int n);

int ceildiv(int num, int mult);
int setupcolors(Colors *colors);
void freecolors(Colors *colors);

#endif
/* vim: set ts=4 sw=4 tw=0 et:*/
