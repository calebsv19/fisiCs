#include <stdint.h>

uintmax_t wave20_stdint_bridge_fold(uintptr_t base, uint32_t step);

int main(void) {
    uintmax_t folded = wave20_stdint_bridge_fold((uintptr_t)0x1200u, UINT32_C(37));
    return (folded % UINTMAX_C(1000)) == UINTMAX_C(645) ? 0 : 1;
}
