#include <errno.h>
#include <stdio.h>

int main(void) {
    int rc;
    int captured;

    errno = 0;
    rc = remove("wave27_missing_file.tmp");
    captured = errno;

    printf("errno-remove rc=%d captured=%d positive=%d\n", rc, captured, captured > 0);
    return rc < 0 && captured > 0 ? 0 : 1;
}
