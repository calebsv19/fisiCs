#include <stdio.h>

#include "13__probe_wave58_for_header_recovery_aggregate_contract.h"

int main(void) {
    struct wave58_payload built = wave58_build_payload(11);
    struct wave58_payload copied = built;
    long checksum;

    copied.lane[1] += copied.meta.left;
    checksum = copied.lane[0] + copied.lane[1] + copied.lane[2] +
               copied.lane[3] + copied.meta.left + copied.meta.right;
    printf("wave58:%ld:%ld:%d:%d\n",
           checksum,
           copied.lane[3],
           copied.meta.left,
           copied.meta.right);
    return checksum == 192L ? 0 : 58;
}
