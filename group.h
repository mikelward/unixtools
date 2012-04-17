#ifndef GROUP_H
#define GROUP_H

#include <sys/types.h>
#include <grp.h>

#include "logging.h"

typedef struct GROUP {
	gid_t gid;
	char *groupname;
} Group;

Group *newgroup(gid_t gid);
char *getgroupname(Group *group);

#endif