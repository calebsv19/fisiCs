#include "11__probe_wave92_typedef_payload_table_reverse_contract.h"

static Wave92Payload wave92_add(Wave92Payload seed,
                                long long *delta,
                                int bias) {
    Wave92Payload out = seed;
    int index;
    int total = 0;

    for (index = 0; index < 3; ++index) {
        out.lane[index] += delta[index] + bias;
        total += (int)delta[index];
    }
    out.stamp += total + bias;
    return out;
}

static Wave92Payload wave92_mix(Wave92Payload seed,
                                long long delta[],
                                int bias) {
    Wave92Payload out = seed;

    out.lane[0] = seed.lane[0] * 2 + delta[2] + bias;
    out.lane[1] = seed.lane[1] * 2 + delta[1] - bias;
    out.lane[2] = seed.lane[2] * 2 + delta[0] + bias;
    out.stamp = seed.stamp - (int)delta[0] + bias;
    return out;
}

const Wave92TransformTable *wave92_get_table(int reverse) {
    static Wave92TransformTable forward = {wave92_add, wave92_mix};
    static Wave92TransformTable backward = {wave92_mix, wave92_add};

    return reverse ? &backward : &forward;
}
