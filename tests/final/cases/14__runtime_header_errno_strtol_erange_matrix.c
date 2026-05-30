#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    char *tail = 0;
    char *overflow_tail = 0;
    long parsed = 0;
    long clipped = 0;
    int parse_errno = 0;
    int range_errno = 0;

    errno = 0;
    parsed = strtol("-42x", &tail, 10);
    parse_errno = errno;

    errno = 0;
    clipped = strtol("999999999999999999999999999999999999", &overflow_tail, 10);
    range_errno = errno;

    if (!tail || *tail != 'x' || parsed != -42L || parse_errno != 0) {
        return 1;
    }
    if (!overflow_tail || *overflow_tail != '\0') {
        return 2;
    }
    if (clipped != LONG_MAX || range_errno != ERANGE) {
        return 3;
    }

    printf(
        "value=%ld tail=%c clipped=%ld range=%d max=%ld\n",
        parsed,
        *tail,
        clipped,
        range_errno,
        (long)LONG_MAX);
    return 0;
}
