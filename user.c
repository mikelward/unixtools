
#include <sys/types.h>
#include <pwd.h>
#include <stdlib.h>

#include "logging.h"

char *getusername(uid_t uid)
{
    struct passwd *ppwd = getpwuid(uid);
    if (!ppwd) {
        errorf("User %lu not found\n", (unsigned long)uid);
        return NULL;
    }
    return ppwd->pw_name;
}
