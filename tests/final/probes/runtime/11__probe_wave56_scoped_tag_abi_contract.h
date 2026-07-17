#ifndef FISICS_PROBE_WAVE56_SCOPED_TAG_ABI_CONTRACT_H
#define FISICS_PROBE_WAVE56_SCOPED_TAG_ABI_CONTRACT_H

typedef struct wave56_scoped_tag_abi_payload {
    long long lane;
    long long total;
    long long trace;
    int stamp;
    int guard;
} Wave56ScopedTagAbiPayload;

typedef Wave56ScopedTagAbiPayload (*Wave56ScopedTagAbiTransform)(
    Wave56ScopedTagAbiPayload value,
    int salt);

Wave56ScopedTagAbiPayload wave56_scoped_tag_abi_make(int seed, int bias);

Wave56ScopedTagAbiPayload wave56_scoped_tag_abi_forward(
    Wave56ScopedTagAbiPayload value,
    Wave56ScopedTagAbiTransform transform,
    int salt);

#endif
