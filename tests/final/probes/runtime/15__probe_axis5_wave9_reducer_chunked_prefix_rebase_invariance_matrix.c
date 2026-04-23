#include <stdio.h>

typedef struct Axis5W9Entry {
    unsigned int key;
    int value;
} Axis5W9Entry;

static unsigned int axis5_w9_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w9_decode_direct(const int* delta, int n, Axis5W9Entry* out) {
    int acc = 0;
    for (int i = 0; i < n; ++i) {
        acc += delta[i];
        out[i].key = (unsigned int)i & 15u;
        out[i].value = acc;
    }
}

static void axis5_w9_decode_chunked(
    const int* delta,
    int n,
    int chunk,
    Axis5W9Entry* out
) {
    int base = 0;
    for (int b = 0; b < n; b += chunk) {
        int local = base;
        int limit = (b + chunk < n) ? (b + chunk) : n;
        for (int i = b; i < limit; ++i) {
            local += delta[i];
            out[i].key = (unsigned int)i & 15u;
            out[i].value = local;
        }
        base = local;
    }
}

static unsigned int axis5_w9_sig(const Axis5W9Entry* rows, int n) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < n; ++i) {
        h = axis5_w9_mix(h, rows[i].key & 31u);
        h = axis5_w9_mix(h, (unsigned int)(rows[i].value & 0xffff));
    }
    return h;
}

int main(void) {
    const int delta[] = {7, -2, 5, -1, 4, -3, 6, -2, 3, 1, -4, 8};
    Axis5W9Entry direct[12];
    Axis5W9Entry chunked[12];

    axis5_w9_decode_direct(delta, 12, direct);
    axis5_w9_decode_chunked(delta, 12, 3, chunked);

    const unsigned int sig_direct = axis5_w9_sig(direct, 12);
    const unsigned int sig_chunked = axis5_w9_sig(chunked, 12);
    const unsigned int same = (sig_direct == sig_chunked) ? 1u : 0u;

    printf("%u %u %u\n", sig_direct, sig_chunked, same);
    return 0;
}
