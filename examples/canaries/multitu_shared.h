// SPDX-License-Identifier: Apache-2.0

#ifndef FISICS_EXAMPLES_CANARIES_MULTITU_SHARED_H
#define FISICS_EXAMPLES_CANARIES_MULTITU_SHARED_H

typedef struct CanaryRecord {
    int base;
    int adjusted;
    const char* label;
} CanaryRecord;

CanaryRecord canary_make_record(int seed);

#endif
