#ifndef PAIR_H
#define PAIR_H

typedef struct pair Pair;

Pair *newpair(int key, char *value);
void freepair(Pair *pair);
int getkey(Pair *pair);
char *getvalue(Pair *pair);
void setvalue(Pair *pair, char *value);

#endif