#include <stdio.h>

#include "10__probe_multitu_include_struct_strong_def_bridge_runtime.h"

struct Bucket10HeaderStrongBridge bucket10_header_strong_bridge = {7, 5, 3};

int main(void) {
    printf("%d ", bucket10_header_strong_bridge_score());
    bucket10_header_strong_bridge_shift(4);
    printf("%d %d\n",
           bucket10_header_strong_bridge_score(),
           bucket10_header_strong_bridge.base + bucket10_header_strong_bridge.scale + bucket10_header_strong_bridge.bias);
    return 0;
}
