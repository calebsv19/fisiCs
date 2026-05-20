#include "10__probe_multitu_include_extern_scalar_bridge_runtime.h"

int bucket10_header_scalar_score(void) {
    return bucket10_header_scalar_bridge + 7;
}

void bucket10_header_scalar_shift(int delta) {
    bucket10_header_scalar_bridge += delta - 2;
}
