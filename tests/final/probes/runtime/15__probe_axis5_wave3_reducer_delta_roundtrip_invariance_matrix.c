#include <stdio.h>

typedef struct Axis5W3Cell {
    unsigned int key;
    int value;
} Axis5W3Cell;

static unsigned int axis5_w3_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static unsigned int axis5_w3_signature(const Axis5W3Cell* cells, int n) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < n; ++i) {
        h = axis5_w3_mix(h, cells[i].key & 0xffu);
        h = axis5_w3_mix(h, (unsigned int)(cells[i].value & 0xffff));
    }
    return h;
}

static void axis5_w3_encode_delta(const Axis5W3Cell* in, int n, int* out_delta) {
    int prev = 0;
    for (int i = 0; i < n; ++i) {
        out_delta[i] = in[i].value - prev;
        prev = in[i].value;
    }
}

static void axis5_w3_decode_delta(
    const Axis5W3Cell* proto,
    const int* delta,
    int n,
    Axis5W3Cell* out
) {
    int prev = 0;
    for (int i = 0; i < n; ++i) {
        prev += delta[i];
        out[i].key = proto[i].key;
        out[i].value = prev;
    }
}

int main(void) {
    const Axis5W3Cell baseline[] = {
        {2u, 9}, {4u, 13}, {1u, 26}, {3u, 18},
        {7u, 31}, {0u, 33}, {5u, 30}, {6u, 44},
    };
    int delta[8];
    Axis5W3Cell decoded[8];

    axis5_w3_encode_delta(baseline, 8, delta);
    axis5_w3_decode_delta(baseline, delta, 8, decoded);

    const unsigned int sig_a = axis5_w3_signature(baseline, 8);
    const unsigned int sig_b = axis5_w3_signature(decoded, 8);
    const unsigned int same = (sig_a == sig_b) ? 1u : 0u;

    printf("%u %u %u\n", sig_a, sig_b, same);
    return 0;
}
