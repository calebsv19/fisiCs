#include "10__probe_multitu_include_extern_array_bridge_runtime.h"

int accumulate_values(void) {
    return values[0] + values[1] + values[2] + values[3];
}

void tweak_value(int index, int delta) {
    values[index] += delta;
}
