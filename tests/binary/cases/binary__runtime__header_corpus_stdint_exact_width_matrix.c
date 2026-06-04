#include <stdint.h>
#include <stdio.h>

int main(void) {
    int ok = 1;
    uint64_t wide = (uint64_t)UINT32_MAX + UINT64_C(1);

    ok = ok && INT8_MIN == -128;
    ok = ok && UINT8_MAX == UINT8_C(255);
    ok = ok && INT16_MIN == -32768;
    ok = ok && UINT16_MAX == UINT16_C(65535);
    ok = ok && INT32_MAX == INT32_C(2147483647);
    ok = ok && wide == UINT64_C(4294967296);

    printf("stdint-exact ok=%d wide=%llu low=%u\n",
           ok,
           (unsigned long long)wide,
           (unsigned)UINT16_MAX);
    return ok ? 0 : 1;
}
