#include "10__probe_multitu_include_extern_struct_bridge_runtime.h"

int bucket10_bridge_score(void) {
    return bucket10_bridge_state.base * bucket10_bridge_state.step + bucket10_bridge_state.bias;
}

void bucket10_bridge_shift(int delta) {
    bucket10_bridge_state.base += delta;
    bucket10_bridge_state.step += 1;
    bucket10_bridge_state.bias -= delta;
}
