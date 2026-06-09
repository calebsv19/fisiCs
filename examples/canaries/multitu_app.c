// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

#include "multitu_shared.h"

int main(void) {
    CanaryRecord record = canary_make_record(7);
    printf("canary multitu: base=%d adjusted=%d label=%s\n",
           record.base,
           record.adjusted,
           record.label);
    return (record.base == 14 && record.adjusted == 17) ? 0 : 1;
}
