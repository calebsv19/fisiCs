#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    int sign_ok = (INTPTR_MIN < 0) && (INTPTR_MAX > 0);
    int order_ok = ((uintptr_t)INTPTR_MAX) <= UINTPTR_MAX;
    int bits = (int)(sizeof(intptr_t) * CHAR_BIT);

    uintmax_t umax = (uintmax_t)UINTPTR_MAX;
    uintmax_t smax = (uintmax_t)(uintptr_t)INTPTR_MAX;
    uintmax_t span = umax - smax;

    printf("%d %d %d %ju\n", sign_ok, order_ok, bits, span % (uintmax_t)1000003u);
    return 0;
}
