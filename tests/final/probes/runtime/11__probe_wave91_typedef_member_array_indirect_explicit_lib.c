#include "11__probe_wave91_typedef_member_array_indirect_contract.h"

static Wave91Payload wave91_add_samples(int *samples, Wave91Payload seed) {
    Wave91Payload out = seed;
    int total = 0;
    int index;

    for (index = 0; index < 3; ++index) {
        out.lane[index] += samples[index];
        total += samples[index];
    }
    out.stamp += total;
    return out;
}

static Wave91Payload wave91_scale_reverse(int samples[], Wave91Payload seed) {
    Wave91Payload out = seed;

    out.lane[0] = seed.lane[0] * 2 + samples[2];
    out.lane[1] = seed.lane[1] * 2 + samples[1];
    out.lane[2] = seed.lane[2] * 2 + samples[0];
    out.stamp -= samples[0];
    return out;
}

Wave91Dispatch wave91_make_dispatch(int reverse) {
    Wave91Dispatch dispatch;

    dispatch.transforms[reverse ? 1 : 0] = wave91_add_samples;
    dispatch.transforms[reverse ? 0 : 1] = wave91_scale_reverse;
    dispatch.generation = 91;
    return dispatch;
}
