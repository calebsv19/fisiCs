#include <stdio.h>

#include "10__probe_multitu_include_extern_struct_bridge_runtime.h"

struct Bucket10BridgeState bucket10_bridge_state = {6, 4, 2};

int main(void) {
    printf("%d ", bucket10_bridge_score());
    bucket10_bridge_shift(3);
    printf("%d %d\n",
           bucket10_bridge_score(),
           bucket10_bridge_state.base + bucket10_bridge_state.step + bucket10_bridge_state.bias);
    return 0;
}
