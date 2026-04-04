#include <errno.h>
#include <stdlib.h>

int main(void) {
    const char* raw = "xyz";
    char* end = 0;
    unsigned long value;
    int consumed;

    errno = 0;
    value = strtoul(raw, &end, 10);
    if (!end) {
        return 1;
    }
    consumed = (int)(end - raw);
    if (value != 0ul || consumed != 0) {
        return 2;
    }

    /* impl-defined policy: libc may leave errno=0 or set EINVAL for no digits. */
    if (errno != 0 && errno != EINVAL) {
        return 3;
    }
    return 0;
}
