
#include <sys/types.h>
#include <grp.h>
#include <stdlib.h>

#include "logging.h"

char *getgroupname(gid_t gid)
{
    struct group *pgrp = getgrgid(gid);
    if (!pgrp) {
        errorf("Group %lu not found\n", (unsigned long)gid);
        return NULL;
    }
    return pgrp->gr_name;
}

