#include "13__probe_wave58_for_header_recovery_aggregate_contract.h"

#ifdef FISICS_WAVE58_RECOVERY
static int wave58_recovered_for_header(int seed) {
    int i;
    for (i = 0 i < 3; ++i) {
        seed += i;
    }
    return seed;
}
#else
static int wave58_recovered_for_header(int seed) {
    int i;
    for (i = 0; i < 3; ++i) {
        seed += i;
    }
    return seed;
}
#endif

struct wave58_payload wave58_build_payload(int seed) {
    struct wave58_payload initial = {
        { seed + 1L, seed + 3L, seed + 5L, seed + 7L },
        { seed * 2, seed * 3 }
    };
    struct wave58_payload copied = initial;

    copied.lane[2] += copied.meta.left;
    copied.lane[3] += copied.meta.right;
    return copied;
}
