// SPDX-License-Identifier: Apache-2.0

/*
 * Current-threshold companion for the strict EDU-12-shaped probe.
 *
 * Counting down from the incoming step value avoids the compiler's current
 * eight-byte zero-initialization lowering to an undefined freestanding
 * memset helper. The arithmetic order and public ABI remain unchanged.
 */

typedef unsigned long osp2_u64;

double osp2_simulate_partition(
    double position,
    double velocity,
    double acceleration,
    osp2_u64 steps
) {
    osp2_u64 remaining = steps;
    while (remaining != 0) {
        velocity = velocity + acceleration;
        position = position + velocity;
        remaining = remaining - 1;
    }
    return position;
}

osp2_u64 osp2_reduce_result(
    osp2_u64 left_bits,
    osp2_u64 right_bits,
    osp2_u64 seed
) {
    osp2_u64 value = seed ^ left_bits;
    value = (value << 13) | (value >> 51);
    value = value + right_bits;
    return value ^ 0xD1B54A32D192ED03UL;
}
