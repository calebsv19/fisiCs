#include <limits.h>
#include <stdio.h>

int main(void) {
    unsigned u = 0u - 1u;
    printf("%d %u %u\n", u == UINT_MAX, u, UINT_MAX);
    return 0;
}
