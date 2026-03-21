#ifndef PAIR_H
#define PAIR_H

#include <inttypes.h>

typedef struct pair Pair;

Pair *newpair(uintmax_t key, char *value);
void freepair(Pair *pair);
uintmax_t getkey(Pair *pair);
char *getvalue(Pair *pair);
void setvalue(Pair *pair, char *value);

#endif