#include <stdio.h>

#include "10__probe_multitu_include_extern_scalar_bridge_runtime.h"

int bucket10_header_scalar_bridge = 11;

int main(void) {
    int before;
    int after;
    int raw;

    before = bucket10_header_scalar_score();
    bucket10_header_scalar_shift(5);
    after = bucket10_header_scalar_score();
    raw = bucket10_header_scalar_bridge;
    printf("%d %d %d\n", before, after, raw);
    return 0;
}
