#include "15__probe_axis16_wave1_runtime_clang_gcc16_tri_diff_multitu_volatile_access_order_shared.h"

void axis16_store(volatile unsigned *slot, unsigned value) {
    *slot = value;
}

unsigned axis16_load(const volatile unsigned *slot) {
    return *slot;
}
