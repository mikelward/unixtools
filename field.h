#ifndef FIELD_H
#define FIELD_H

enum align { ALIGN_NONE, ALIGN_LEFT, ALIGN_RIGHT };

typedef struct field {
    const char *string;
    enum align align;
    int width : 8;
} Field;

Field *newfield(const char *string, enum align align, int width);
void freefield(Field *field);
enum align fieldalign(Field *field);
const char *fieldstring(Field *field);
int fieldwidth(Field *field);

#endif
/* vim: set ts=4 sw=4 tw=0 et:*/
