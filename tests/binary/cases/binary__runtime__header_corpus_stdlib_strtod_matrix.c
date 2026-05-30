#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    char *tail_a = 0;
    char *tail_b = 0;
    double first = 0.0;
    double second = 0.0;

    errno = 0;
    first = strtod("12.5x", &tail_a);
    if (errno != 0 || !tail_a || *tail_a != 'x') {
        return 1;
    }

    errno = 0;
    second = strtod("-0.125?", &tail_b);
    if (errno != 0 || !tail_b || *tail_b != '?') {
        return 2;
    }

    printf(
        "first=%.3f tail=%c second=%.3f tail2=%c\n",
        first,
        *tail_a,
        second,
        *tail_b);
    return 0;
}
