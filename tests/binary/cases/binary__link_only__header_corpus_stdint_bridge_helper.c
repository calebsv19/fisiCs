#include <stdint.h>

uintmax_t wave20_stdint_bridge_fold(uintptr_t base, uint32_t step) {
    uintmax_t total = (uintmax_t)(base & (uintptr_t)0xffu);
    total += (uintmax_t)UINT8_MAX;
    total += (uintmax_t)UINT16_MAX;
    total += (uintmax_t)(step * UINT32_C(17));
    return total;
}
