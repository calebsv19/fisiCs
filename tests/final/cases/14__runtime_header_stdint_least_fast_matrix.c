#include <stdint.h>
#include <stdio.h>

int main(void) {
    int_least16_t signed_part = INT16_C(-30000);
    uint_least16_t unsigned_part = UINT16_C(60000);
    int_fast32_t fast_signed = INT32_C(1234567);
    uint_fast32_t fast_unsigned = UINT32_C(4000000000);
    uintmax_t total = (uintmax_t)unsigned_part;

    total += (uintmax_t)(-signed_part);
    total += (uintmax_t)(fast_signed % INT32_C(1000));
    total += (uintmax_t)(fast_unsigned % UINT32_C(1000));

    printf("stdint-least-fast total=%ju\n", total);
    return total == UINTMAX_C(90567) ? 0 : 1;
}
