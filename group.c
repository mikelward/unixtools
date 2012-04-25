#define _XOPEN_SOURCE 600       /* for strdup() */

#include "group.h"

#include <stdlib.h>
#include <string.h>

Group *newgroup(gid_t gid)
{
	Group *group = malloc(sizeof *group);
	if (group == NULL) {
		errorf("out of memory\n");
		return NULL;
	}

	struct group *pgrp = getgrgid(gid);
	if (pgrp == NULL) {
		errorf("Group %u not found\n", (unsigned)gid);
		return NULL;
	}

	group->gid = pgrp->gr_gid;

	group->groupname = strdup(pgrp->gr_name);
	if (group->groupname == NULL) {
		errorf("strdup returned NULL\n");
		return NULL;
	}

	return group;
}

char *getgroupname(Group *group)
{
	if (group == NULL) {
		errorf("user is NULL\n");
		return NULL;
	}

	return group->groupname;
}
