#include <stdarg.h>
#include <stdio.h>

static unsigned axis13_dual_fold(unsigned count, ...) {
    va_list forward;
    va_list replay;
    unsigned i;
    unsigned fold = 17u;
    unsigned weighted = 0u;

    va_start(forward, count);
    va_copy(replay, forward);
    for (i = 0u; i < count; ++i) {
        fold = fold * 33u + va_arg(forward, unsigned);
    }
    for (i = 0u; i < count; ++i) {
        weighted += va_arg(replay, unsigned) * (i + 3u);
    }
    va_end(replay);
    va_end(forward);
    return fold ^ (weighted << 1u);
}

int main(void) {
    unsigned first = axis13_dual_fold(4u, 7u, 11u, 19u, 23u);
    unsigned second = axis13_dual_fold(5u, 3u, 5u, 8u, 13u, 21u);

    printf("axis13-vacopy=%u,%u\n", first, second);
    return 0;
}
