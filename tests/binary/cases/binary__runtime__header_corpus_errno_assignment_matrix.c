#include <errno.h>
#include <stdio.h>

static int set_errno_value(int value) {
    errno = value;
    return errno;
}

int main(void) {
    int first = set_errno_value(EDOM);
    int second = set_errno_value(ERANGE);
    int total = first + second + (errno == ERANGE);

    printf("errno-assign first=%d second=%d total=%d\n", first, second, total);
    return first == EDOM && second == ERANGE && total == EDOM + ERANGE + 1 ? 0 : 1;
}
