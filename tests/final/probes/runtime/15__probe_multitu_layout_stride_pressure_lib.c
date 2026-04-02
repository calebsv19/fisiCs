#include <stddef.h>

struct Block {
    unsigned char tag;
    unsigned short lane;
    unsigned value;
    unsigned long long stamp;
};

static unsigned mix32(unsigned x) {
    x ^= x >> 16u;
    x *= 0x7feb352du;
    x ^= x >> 15u;
    x *= 0x846ca68bu;
    x ^= x >> 16u;
    return x;
}

unsigned mt27_layout_digest(const struct Block *arr, unsigned count) {
    unsigned state = 0x811c9dc5u;

    for (unsigned i = 0u; i < count; ++i) {
        unsigned lane = (unsigned)arr[i].lane;
        unsigned tag = (unsigned)arr[i].tag;
        unsigned value = arr[i].value;
        unsigned long long stamp = arr[i].stamp;
        unsigned upper = (unsigned)(stamp >> 32u);
        unsigned lower = (unsigned)(stamp & 0xffffffffu);
        unsigned payload = tag * 257u + lane * 17u + value ^ upper ^ (lower << 1u);
        state = (state ^ mix32(payload + i * 131u)) * 16777619u;
    }

    return state;
}

unsigned mt27_layout_stride_score(const struct Block *arr, unsigned count) {
    unsigned stride = (unsigned)((const char *)(arr + 1) - (const char *)arr);
    unsigned off_lane = (unsigned)offsetof(struct Block, lane);
    unsigned off_value = (unsigned)offsetof(struct Block, value);
    unsigned off_stamp = (unsigned)offsetof(struct Block, stamp);
    return stride * count + off_lane * 3u + off_value * 5u + off_stamp * 7u;
}
