#include <errno.h>
#include <stdio.h>

int wave27_errno_bridge_score(const char *path) {
    int rc;

    errno = 0;
    rc = remove(path);
    return rc < 0 ? errno : 0;
}
