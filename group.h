#ifndef GROUP_H
#define GROUP_H

#include <sys/types.h>
#include <stdlib.h>

/**
 * Get the groupname of gid.
 *
 * Caller must NOT free the returned value.
 */
char *getgroupname(gid_t gid);

#endif