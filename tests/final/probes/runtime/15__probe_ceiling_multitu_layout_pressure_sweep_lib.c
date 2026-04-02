#include <stddef.h>

struct LayoutNode {
    unsigned char tag;
    unsigned short lane;
    unsigned value;
    unsigned long long stamp;
    int bias;
};

static unsigned mix32(unsigned x) {
    x ^= x >> 16u;
    x *= 0x7feb352du;
    x ^= x >> 15u;
    x *= 0x846ca68bu;
    x ^= x >> 16u;
    return x;
}

unsigned mt36_layout_digest(const struct LayoutNode *arr, unsigned count) {
    unsigned state = 0x811c9dc5u;

    for (unsigned i = 0u; i < count; ++i) {
        unsigned payload =
            (unsigned)arr[i].tag * 257u +
            (unsigned)arr[i].lane * 19u +
            arr[i].value ^
            (unsigned)(arr[i].stamp >> 32u) ^
            (unsigned)arr[i].bias;
        state = (state ^ mix32(payload + i * 131u)) * 16777619u;
    }

    return state;
}

unsigned mt36_layout_stride_score(const struct LayoutNode *arr, unsigned count) {
    unsigned stride = (unsigned)((const char *)(arr + 1) - (const char *)arr);
    unsigned off_lane = (unsigned)offsetof(struct LayoutNode, lane);
    unsigned off_value = (unsigned)offsetof(struct LayoutNode, value);
    unsigned off_stamp = (unsigned)offsetof(struct LayoutNode, stamp);
    unsigned off_bias = (unsigned)offsetof(struct LayoutNode, bias);

    return stride * count + off_lane * 3u + off_value * 5u + off_stamp * 7u + off_bias * 11u;
}

unsigned mt36_layout_cross_fold(const struct LayoutNode *arr, unsigned count) {
    unsigned acc = 0x9e3779b9u;

    for (unsigned i = 0u; i < count; i += 7u) {
        unsigned v = arr[i].value ^ (unsigned)arr[i].tag ^ (unsigned)arr[i].lane;
        v ^= (unsigned)(arr[i].stamp & 0xffffffffu);
        acc ^= mix32(v + i * 17u);
        acc = acc * 2654435761u + 97u;
    }

    return acc ^ (acc >> 11u);
}
