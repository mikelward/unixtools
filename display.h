#ifndef DISPLAY_H
#define DISPLAY_H

#include "list.h"

typedef List StringList;

void printacross(StringList *list, int stringwidth, int screenwidth);
void printdown(StringList *list, int stringwidth, int screenwidth);

#endif
/* vim: set ts=4 sw=4 tw=0 et:*/
