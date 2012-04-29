#ifndef FILEFIELDS_H
#define FILEFIELDS_H

#include "field.h"
#include "file.h"
#include "list.h"
#include "options.h"

typedef List FieldList;             /* list of fields for a single file */

/**
 * Returns a list of Fields for the given file.
 *
 * Which fields are returned is controlled by options.
 */
FieldList *getfilefields(File *file, Options *options);

Field *getbytesfield(File *file, Options *options, char *buf, int bufsize);
Field *getdatetimefield(File *file, Options *options, char *buf, int bufsize);
Field *getgroupfield(File *file, Options *options, char *buf, int bufsize);
Field *getinodefield(File *file, Options *options, char *buf, int bufsize);
Field *getlinkfield(File *file, Options *options, char *buf, int bufsize);
Field *getmodesfield(File *file, Options *options, char *buf, int bufsize);
Field *getnamefield(File *file, Options *options);
Field *getownerfield(File *file, Options *options, char *buf, int bufsize);
Field *getpermsfield(File *file, Options *options, char *buf, int bufsize);
Field *getsizefield(File *file, Options *options, char *buf, int bufsize);

#endif