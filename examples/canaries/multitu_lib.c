// SPDX-License-Identifier: Apache-2.0

#include "multitu_shared.h"

CanaryRecord canary_make_record(int seed) {
    CanaryRecord record;
    record.base = seed * 2;
    record.adjusted = record.base + 3;
    record.label = "ok";
    return record;
}
