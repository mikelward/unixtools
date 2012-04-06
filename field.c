#include "field.h"
#include "logging.h"

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void freefield(Field *field)
{
    if (field == NULL) {
        errorf(__func__, "field is NULL\n");
        return;
    }
    if (field->string != NULL) {
        /* XXX is const qualifier correct? */
        free((char *)field->string);
    }
    free(field);
}

Field *newfield(const char *string, enum align align, int width)
{
    Field *fp = malloc(sizeof *fp);
    if (fp == NULL) {
        errorf(__func__, "fp is NULL: out of memory\n");
        return NULL;
    }
    fp->string = strdup(string);
    if (fp->string == NULL) {
        errorf(__func__, "fp->string is NULL: out of memory\n");
        return NULL;
    }
    fp->align = align;
    fp->width = width;
    return fp;
}

int fieldwidth(Field *field)
{
    if (field == NULL) {
        errorf(__func__, "field is NULL\n");
        return 0;
    }
    return field->width;
}

enum align fieldalign(Field *field)
{
    if (field == NULL) {
        errorf(__func__, "field is NULL\n");
        return 0;
    }
    return field->align;
}

const char *fieldstring(Field *field)
{
    if (field == NULL) {
        errorf(__func__, "field is NULL\n");
        return 0;
    }
    return field->string;
}


/* vim: set ts=4 sw=4 tw=0 et:*/
