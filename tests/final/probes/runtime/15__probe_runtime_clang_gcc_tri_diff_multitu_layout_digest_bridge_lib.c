#include <stddef.h>

struct Span {
    unsigned char lane;
    unsigned int payload;
    unsigned short mark;
};

static unsigned mix32(unsigned x) {
    x ^= x >> 16u;
    x *= 0x45d9f3bu;
    x ^= x >> 16u;
    x *= 0x45d9f3bu;
    x ^= x >> 16u;
    return x;
}

unsigned w25_layout_digest(const struct Span *arr, unsigned count) {
    unsigned state = 0x811c9dc5u;

    for (unsigned i = 0u; i < count; ++i) {
        unsigned lane = (unsigned)arr[i].lane;
        unsigned payload = arr[i].payload;
        unsigned mark = (unsigned)arr[i].mark;
        unsigned v = lane * 257u + payload ^ (mark << 11u);
        state = (state ^ mix32(v + i * 131u)) * 16777619u;
    }

    return state;
}

unsigned w25_layout_stride_bytes(const struct Span *arr, unsigned count) {
    unsigned stride = (unsigned)((const char *)(arr + 1) - (const char *)arr);
    unsigned total = stride * count;
    return total + (unsigned)offsetof(struct Span, payload);
}
