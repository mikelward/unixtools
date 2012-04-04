#include "field.h"

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void freefield(Field *field)
{
    if (field == NULL) {
        fprintf(stderr, "freefield: field is NULL\n");
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
        fprintf(stderr, "newfield: fp is NULL: out of memory\n");
        return NULL;
    }
    fp->string = strdup(string);
    if (fp->string == NULL) {
        fprintf(stderr, "newfield: fp->string is NULL: out of memory\n");
        return NULL;
    }
    fp->align = align;
    fp->width = width;
    return fp;
}

int fieldwidth(Field *field)
{
    if (field == NULL) {
        fprintf(stderr, "fieldwidth: field is NULL\n");
        return 0;
    }
    return field->width;
}

enum align fieldalign(Field *field)
{
    if (field == NULL) {
        fprintf(stderr, "fieldalign: field is NULL\n");
        return 0;
    }
    return field->align;
}

const char *fieldstring(Field *field)
{
    if (field == NULL) {
        fprintf(stderr, "fieldstring: field is NULL\n");
        return 0;
    }
    return field->string;
}


/* vim: set ts=4 sw=4 tw=0 et:*/
