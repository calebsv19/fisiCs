#include <stdio.h>

typedef struct Axis5W7Accum {
    unsigned int bins[4];
    unsigned int xor_mix;
    unsigned int token;
} Axis5W7Accum;

void axis5_w7_fold_segment(
    Axis5W7Accum* out,
    const unsigned int* lanes,
    const unsigned int* values,
    int n
);
void axis5_w7_encode_frontier(const Axis5W7Accum* a, unsigned int wire[6]);
void axis5_w7_decode_frontier(Axis5W7Accum* a, const unsigned int wire[6]);
void axis5_w7_merge(Axis5W7Accum* dst, const Axis5W7Accum* src);
unsigned int axis5_w7_signature(const Axis5W7Accum* a);

int main(void) {
    const unsigned int lanes[] = {0u, 1u, 3u, 2u, 1u, 0u, 2u, 3u, 0u, 2u, 1u, 3u};
    const unsigned int vals[] = {4u, 7u, 5u, 3u, 9u, 2u, 8u, 6u, 11u, 1u, 10u, 12u};

    Axis5W7Accum full;
    Axis5W7Accum s0;
    Axis5W7Accum s1;
    Axis5W7Accum s2;
    Axis5W7Accum d0;
    Axis5W7Accum d1;
    Axis5W7Accum d2;
    Axis5W7Accum m_left;
    Axis5W7Accum m_right;
    unsigned int w0[6];
    unsigned int w1[6];
    unsigned int w2[6];

    axis5_w7_fold_segment(&full, lanes, vals, 12);
    axis5_w7_fold_segment(&s0, lanes, vals, 4);
    axis5_w7_fold_segment(&s1, lanes + 4, vals + 4, 4);
    axis5_w7_fold_segment(&s2, lanes + 8, vals + 8, 4);

    axis5_w7_encode_frontier(&s0, w0);
    axis5_w7_encode_frontier(&s1, w1);
    axis5_w7_encode_frontier(&s2, w2);
    axis5_w7_decode_frontier(&d0, w0);
    axis5_w7_decode_frontier(&d1, w1);
    axis5_w7_decode_frontier(&d2, w2);

    m_left = d0;
    axis5_w7_merge(&m_left, &d1);
    axis5_w7_merge(&m_left, &d2);

    m_right = d1;
    axis5_w7_merge(&m_right, &d2);
    axis5_w7_merge(&m_right, &d0);

    const unsigned int sig_full = axis5_w7_signature(&full);
    const unsigned int sig_left = axis5_w7_signature(&m_left);
    const unsigned int sig_right = axis5_w7_signature(&m_right);
    const unsigned int same = (sig_full == sig_left && sig_left == sig_right) ? 1u : 0u;

    printf("%u %u %u %u\n", sig_full, sig_left, sig_right, same);
    return 0;
}
