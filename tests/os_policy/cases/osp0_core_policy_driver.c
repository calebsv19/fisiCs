// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

typedef unsigned char osp0_u8;
typedef unsigned long osp0_u64;

osp0_u64 osp0_read_le64(const osp0_u8* bytes);
osp0_u64 osp0_checked_span(osp0_u64 offset, osp0_u64 length, osp0_u64 capacity);
osp0_u64 osp0_lock_order(osp0_u64 held_rank, osp0_u64 requested_rank);
osp0_u64 osp0_generation_accept(
    osp0_u64 allocated,
    osp0_u64 slot_generation,
    osp0_u64 token_generation
);

static int expect_u64(osp0_u64 actual, osp0_u64 expected) {
    return actual == expected ? 0 : 1;
}
int main(void) {
    static const osp0_u8 bytes[8] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef
    };
    int failures = 0;

    failures += expect_u64(osp0_read_le64(bytes), 0xefcdab8967452301UL);

    failures += expect_u64(osp0_checked_span(0, 0, 0), 1);
    failures += expect_u64(osp0_checked_span(5, 3, 8), 1);
    failures += expect_u64(osp0_checked_span(5, 4, 8), 0);
    failures += expect_u64(osp0_checked_span(9, 0, 8), 0);
    failures += expect_u64(osp0_checked_span(~0UL, 1, ~0UL), 0);

    failures += expect_u64(osp0_lock_order(0, 1), 1);
    failures += expect_u64(osp0_lock_order(0, 3), 1);
    failures += expect_u64(osp0_lock_order(1, 2), 1);
    failures += expect_u64(osp0_lock_order(2, 1), 0);
    failures += expect_u64(osp0_lock_order(3, 3), 0);
    failures += expect_u64(osp0_lock_order(0, 4), 0);

    failures += expect_u64(osp0_generation_accept(1, 7, 7), 1);
    failures += expect_u64(osp0_generation_accept(0, 7, 7), 0);
    failures += expect_u64(osp0_generation_accept(1, 7, 8), 0);
    failures += expect_u64(osp0_generation_accept(1, 0, 0), 0);

    if (failures != 0) {
        puts("OS-P0 case=osp0_core_policy vectors=16 result=FAIL");
        return 1;
    }

    puts("OS-P0 case=osp0_core_policy vectors=16 result=PASS");
    return 0;
}
