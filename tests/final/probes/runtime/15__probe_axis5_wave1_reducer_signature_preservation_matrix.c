#include <stdio.h>

typedef struct Axis5W1Event {
    unsigned int kind;
    int x;
    int y;
    int noise;
} Axis5W1Event;

static unsigned int axis5_w1_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static unsigned int axis5_w1_fold_signature(const Axis5W1Event* events, int count) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < count; ++i) {
        unsigned int kind = events[i].kind & 7u;
        unsigned int nx = (unsigned int)(events[i].x & 0x3ff);
        unsigned int ny = (unsigned int)(events[i].y & 0x3ff);
        /* Noise is intentionally discarded by the reducer signature contract. */
        h = axis5_w1_mix(h, kind);
        h = axis5_w1_mix(h, nx);
        h = axis5_w1_mix(h, ny);
    }
    return h;
}

static void axis5_w1_decode_matrix(const unsigned int* encoded, Axis5W1Event* out, int count) {
    for (int i = 0; i < count; ++i) {
        unsigned int v = encoded[i];
        out[i].kind = (v >> 26) & 0x7u;
        out[i].x = (int)((v >> 13) & 0x3ffu);
        out[i].y = (int)(v & 0x1fffu);
        out[i].noise = (int)((v >> 23) & 0x7u);
    }
}

int main(void) {
    const Axis5W1Event canonical[] = {
        {1u, 17, 91, 7}, {2u, 51, 19, 4}, {4u, 63, 88, 3}, {1u, 20, 94, 9},
        {3u, 70, 13, 2}, {0u, 12, 42, 1}, {6u, 89, 76, 5}, {7u, 34, 22, 8},
    };
#define AX5_W1_PACK(k, x, y, n) \
    ((((unsigned int)(k) & 0x7u) << 26) | (((unsigned int)(n) & 0x7u) << 23) | (((unsigned int)(x) & 0x3ffu) << 13) | ((unsigned int)(y) & 0x1fffu))
    const unsigned int encoded[] = {
        AX5_W1_PACK(1u, 17u, 91u, 7u),
        AX5_W1_PACK(2u, 51u, 19u, 4u),
        AX5_W1_PACK(4u, 63u, 88u, 3u),
        AX5_W1_PACK(1u, 20u, 94u, 1u),
        AX5_W1_PACK(3u, 70u, 13u, 2u),
        AX5_W1_PACK(0u, 12u, 42u, 1u),
        AX5_W1_PACK(6u, 89u, 76u, 5u),
        AX5_W1_PACK(7u, 34u, 22u, 0u),
    };
#undef AX5_W1_PACK

    Axis5W1Event decoded[8];
    axis5_w1_decode_matrix(encoded, decoded, 8);

    const unsigned int sig_a = axis5_w1_fold_signature(canonical, 8);
    const unsigned int sig_b = axis5_w1_fold_signature(decoded, 8);
    const unsigned int parity = (sig_a == sig_b) ? 1u : 0u;

    printf("%u %u %u\n", sig_a, sig_b, parity);
    return 0;
}
