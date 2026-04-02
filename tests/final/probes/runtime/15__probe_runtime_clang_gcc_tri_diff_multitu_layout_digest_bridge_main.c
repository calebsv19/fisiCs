#include <stdio.h>

struct Span {
    unsigned char lane;
    unsigned int payload;
    unsigned short mark;
};

unsigned w25_layout_digest(const struct Span *arr, unsigned count);
unsigned w25_layout_stride_bytes(const struct Span *arr, unsigned count);

int main(void) {
    struct Span spans[9];

    for (unsigned i = 0u; i < 9u; ++i) {
        spans[i].lane = (unsigned char)((i * 3u + 5u) & 0xffu);
        spans[i].payload = (i + 1u) * (i + 11u) * 97u + 0x123u;
        spans[i].mark = (unsigned short)((i * 37u + 19u) & 0xffffu);
    }

    printf("%u %u\n", w25_layout_digest(spans, 9u), w25_layout_stride_bytes(spans, 9u));
    return 0;
}
