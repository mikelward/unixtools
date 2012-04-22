#define _POSIX_C_SOURCE 200809L	/* for strdup() */

#include "user.h"

#include <stdlib.h>
#include <string.h>

User *newuser(uid_t uid)
{
	User *user = malloc(sizeof *user);
	if (user == NULL) {
		errorf(__func__, "out of memory\n");
		return NULL;
	}

	struct passwd *ppwd = getpwuid(uid);
	if (ppwd == NULL) {
		errorf(__func__, "User %u not found\n", (unsigned)uid);
		return NULL;
	}

	user->uid = ppwd->pw_uid;

	user->username = strdup(ppwd->pw_name);
	if (user->username == NULL) {
		errorf(__func__, "strdup returned NULL\n");
		return NULL;
	}

	return user;
}

char *getusername(User *user)
{
	if (user == NULL) {
		errorf(__func__, "user is NULL\n");
		return NULL;
	}

	return user->username;
}