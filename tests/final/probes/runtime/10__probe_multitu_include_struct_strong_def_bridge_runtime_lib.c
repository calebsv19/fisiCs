#include "10__probe_multitu_include_struct_strong_def_bridge_runtime.h"

int bucket10_header_strong_bridge_score(void) {
    return bucket10_header_strong_bridge.base * bucket10_header_strong_bridge.scale + bucket10_header_strong_bridge.bias;
}

void bucket10_header_strong_bridge_shift(int delta) {
    bucket10_header_strong_bridge.base += delta;
    bucket10_header_strong_bridge.scale += 2;
    bucket10_header_strong_bridge.bias -= delta;
}
