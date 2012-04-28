#ifndef USER_H
#define USER_H

#include <sys/types.h>
#include <stdlib.h>

/**
 * Get the username of uid.
 *
 * Caller must NOT free the returned value.
 */
char *getusername(uid_t uid);

#endif