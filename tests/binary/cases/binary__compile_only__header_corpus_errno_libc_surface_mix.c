#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int wave27_errno_libc_surface(const char *text) {
    char *end = 0;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);
    perror("errno-surface");
    return errno + (int)value + (int)(end != 0);
}

int main(void) {
    return 0;
}
