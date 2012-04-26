#include "field.h"
#include "logging.h"

#define _XOPEN_SOURCE 600       /* for strdup() */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void freefield(Field *field)
{
    if (field == NULL) {
        errorf("field is NULL\n");
        return;
    }
    if (field->string != NULL) {
        free(field->string);
    }
    free(field);
}

Field *newfield(const char *string, enum align align, int width)
{
    Field *field = malloc(sizeof *field);
    if (!field) {
        errorf("field is NULL: out of memory?\n");
        return NULL;
    }
    field->string = strdup(string);
    if (!field->string) {
        errorf("field->string is NULL: out of memory?\n");
        return NULL;
    }
    field->align = align;
    field->width = width;
    return field;
}

int fieldwidth(Field *field)
{
    if (field == NULL) {
        errorf("field is NULL\n");
        return 0;
    }
    return field->width;
}

enum align fieldalign(Field *field)
{
    if (field == NULL) {
        errorf("field is NULL\n");
        return 0;
    }
    return field->align;
}

const char *fieldstring(Field *field)
{
    if (field == NULL) {
        errorf("field is NULL\n");
        return 0;
    }
    return field->string;
}


/* vim: set ts=4 sw=4 tw=0 et:*/
