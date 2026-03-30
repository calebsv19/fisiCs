#include <stdio.h>

typedef struct {
    int lo;
    int hi;
    unsigned weight;
} Span;

static int clamp_int(int v, int lo, int hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static unsigned mix_span(unsigned seed, Span s) {
    unsigned x = seed;
    x ^= (unsigned)(s.lo * 131 + s.hi * 17);
    x = (x << 7) | (x >> 25);
    x += s.weight * 2654435761u;
    return x;
}

static unsigned reduce_spans(const Span *spans, unsigned n, unsigned bias) {
    unsigned acc = 2166136261u ^ bias;
    for (unsigned i = 0; i < n; ++i) {
        Span cur = spans[i];
        int mid = clamp_int((cur.lo + cur.hi) / 2, -2000, 2000);
        unsigned norm = (unsigned)(mid + 4096) * (cur.weight + (i + 1u));
        acc ^= mix_span(norm ^ acc, cur);
        acc *= 16777619u;
    }
    return acc;
}

int main(void) {
    Span a[] = {
        { -40, 17, 3u },
        { 9, 80, 5u },
        { -120, -13, 7u },
        { 200, 255, 11u },
        { -7, 133, 13u },
    };
    Span b[] = {
        { -300, -180, 2u },
        { 14, 28, 19u },
        { 77, 310, 23u },
    };

    unsigned ha = reduce_spans(a, (unsigned)(sizeof(a) / sizeof(a[0])), 0x55aa11u);
    unsigned hb = reduce_spans(b, (unsigned)(sizeof(b) / sizeof(b[0])), 0xaa5511u);
    unsigned out = (ha ^ (hb << 1u)) + (ha >> 3u) + (hb >> 5u);

    printf("%u %u %u\n", ha, hb, out);
    return 0;
}
