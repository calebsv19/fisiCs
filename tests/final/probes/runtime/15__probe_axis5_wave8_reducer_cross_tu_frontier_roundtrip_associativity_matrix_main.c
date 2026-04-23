#include <stdio.h>

typedef struct Axis5W8Accum {
    unsigned int lane_sum[4];
    unsigned int xor_mix;
    unsigned int token;
} Axis5W8Accum;

void axis5_w8_fold_segment(
    Axis5W8Accum* out,
    const unsigned int* lanes,
    const unsigned int* values,
    int n
);
void axis5_w8_encode_frontier(const Axis5W8Accum* a, unsigned int wire[6]);
void axis5_w8_decode_frontier(Axis5W8Accum* a, const unsigned int wire[6]);
void axis5_w8_merge(Axis5W8Accum* dst, const Axis5W8Accum* src);
unsigned int axis5_w8_signature(const Axis5W8Accum* a);

int main(void) {
    const unsigned int lanes[] = {0u, 1u, 3u, 2u, 1u, 0u, 2u, 3u, 0u, 2u, 1u, 3u};
    const unsigned int vals[] = {5u, 9u, 4u, 7u, 3u, 8u, 2u, 6u, 11u, 1u, 10u, 12u};

    Axis5W8Accum full;
    Axis5W8Accum s0;
    Axis5W8Accum s1;
    Axis5W8Accum s2;
    Axis5W8Accum d0;
    Axis5W8Accum d1;
    Axis5W8Accum d2;
    Axis5W8Accum left;
    Axis5W8Accum right;
    unsigned int w0[6];
    unsigned int w1[6];
    unsigned int w2[6];

    axis5_w8_fold_segment(&full, lanes, vals, 12);
    axis5_w8_fold_segment(&s0, lanes, vals, 4);
    axis5_w8_fold_segment(&s1, lanes + 4, vals + 4, 4);
    axis5_w8_fold_segment(&s2, lanes + 8, vals + 8, 4);

    axis5_w8_encode_frontier(&s0, w0);
    axis5_w8_encode_frontier(&s1, w1);
    axis5_w8_encode_frontier(&s2, w2);
    axis5_w8_decode_frontier(&d0, w0);
    axis5_w8_decode_frontier(&d1, w1);
    axis5_w8_decode_frontier(&d2, w2);

    left = d0;
    axis5_w8_merge(&left, &d1);
    axis5_w8_merge(&left, &d2);

    right = d2;
    axis5_w8_merge(&right, &d0);
    axis5_w8_merge(&right, &d1);

    const unsigned int sig_full = axis5_w8_signature(&full);
    const unsigned int sig_left = axis5_w8_signature(&left);
    const unsigned int sig_right = axis5_w8_signature(&right);
    const unsigned int same = (sig_full == sig_left && sig_left == sig_right) ? 1u : 0u;

    printf("%u %u %u %u\n", sig_full, sig_left, sig_right, same);
    return 0;
}
