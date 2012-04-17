#ifndef USER_H
#define USER_H

#include <sys/types.h>
#include <pwd.h>

#include "logging.h"

typedef struct user {
	uid_t uid;
	char *username;
} User;

User *newuser(uid_t uid);
char *getusername(User *user);

#endif