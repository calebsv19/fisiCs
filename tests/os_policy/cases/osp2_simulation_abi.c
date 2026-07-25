// SPDX-License-Identifier: Apache-2.0

typedef unsigned long osp2_u64;

double osp2_simulate_partition(
    double position,
    double velocity,
    double acceleration,
    osp2_u64 steps
) {
    osp2_u64 step = 0;
    while (step < steps) {
        velocity = velocity + acceleration;
        position = position + velocity;
        step = step + 1;
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
