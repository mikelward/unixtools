#define _XOPEN_SOURCE 600       /* for strdup() */

#include "user.h"

#include <stdlib.h>
#include <string.h>

User *newuser(uid_t uid)
{
	User *user = malloc(sizeof *user);
	if (user == NULL) {
		errorf("out of memory\n");
		return NULL;
	}

	struct passwd *ppwd = getpwuid(uid);
	if (ppwd == NULL) {
		errorf("User %u not found\n", (unsigned)uid);
		return NULL;
	}

	user->uid = ppwd->pw_uid;

	user->username = strdup(ppwd->pw_name);
	if (user->username == NULL) {
		errorf("strdup returned NULL\n");
		return NULL;
	}

	return user;
}

char *getusername(User *user)
{
	if (user == NULL) {
		errorf("user is NULL\n");
		return NULL;
	}

	return user->username;
}
