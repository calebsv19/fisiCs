#include "11__probe_wave56_scoped_tag_abi_contract.h"

Wave56ScopedTagAbiPayload wave56_scoped_tag_abi_make(int seed, int bias) {
    Wave56ScopedTagAbiPayload out;

    {
        struct wave56_scoped_tag_abi_payload {
            int lane;
            short total;
            unsigned char stamp;
            unsigned char guard;
        };
        typedef struct wave56_scoped_tag_abi_payload Wave56ScopedTagAbiPayload;
        Wave56ScopedTagAbiPayload local;

        local.lane = seed + bias;
        local.total = (short)(seed * 2 - bias);
        local.stamp = (unsigned char)(seed + 3);
        local.guard = (unsigned char)(bias + 5);

        out.lane = (long long)local.lane * 13 + local.stamp;
        out.total = (long long)local.total * 17 + local.guard;
        out.trace = out.lane + out.total + local.stamp;
        out.stamp = (int)local.stamp * 3;
        out.guard = (int)local.guard * 5;
    }

    return out;
}

Wave56ScopedTagAbiPayload wave56_scoped_tag_abi_forward(
    Wave56ScopedTagAbiPayload value,
    Wave56ScopedTagAbiTransform transform,
    int salt) {
    Wave56ScopedTagAbiPayload out = value;

    {
        struct wave56_scoped_tag_abi_payload {
            int delta;
            int fold;
            unsigned char mark;
            unsigned char guard;
        };
        typedef struct wave56_scoped_tag_abi_payload Wave56ScopedTagAbiPayload;
        Wave56ScopedTagAbiPayload local;

        local.delta = salt + 2;
        local.fold = value.stamp - salt;
        local.mark = 3;
        local.guard = (unsigned char)(value.guard + 1);

        out = transform(out, local.delta);
        out.lane += local.fold;
        out.total += (long long)local.mark * 11;
        out.trace += local.guard;
        out.stamp += local.delta;
        out.guard += local.mark;
    }

    return out;
}
