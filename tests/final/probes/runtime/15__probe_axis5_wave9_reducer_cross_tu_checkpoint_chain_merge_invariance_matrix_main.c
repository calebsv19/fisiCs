#include <stdio.h>

typedef struct Axis5W9Accum {
    unsigned int bins[4];
    unsigned int xormix;
    unsigned int token;
} Axis5W9Accum;

void axis5_w9_fold_segment(
    Axis5W9Accum* out,
    const unsigned int* lanes,
    const unsigned int* values,
    int n
);
void axis5_w9_encode_checkpoint(const Axis5W9Accum* a, unsigned int wire[6]);
void axis5_w9_decode_checkpoint(Axis5W9Accum* a, const unsigned int wire[6]);
void axis5_w9_merge(Axis5W9Accum* dst, const Axis5W9Accum* src);
unsigned int axis5_w9_signature(const Axis5W9Accum* a);

int main(void) {
    const unsigned int lanes[] = {0u, 1u, 3u, 2u, 1u, 0u, 2u, 3u, 0u, 2u, 1u, 3u};
    const unsigned int vals[] = {6u, 4u, 9u, 3u, 7u, 2u, 8u, 5u, 11u, 1u, 10u, 12u};

    Axis5W9Accum full;
    Axis5W9Accum s0;
    Axis5W9Accum s1;
    Axis5W9Accum s2;
    Axis5W9Accum d0;
    Axis5W9Accum d1;
    Axis5W9Accum d2;
    Axis5W9Accum merged;
    unsigned int w0[6];
    unsigned int w1[6];
    unsigned int w2[6];

    axis5_w9_fold_segment(&full, lanes, vals, 12);
    axis5_w9_fold_segment(&s0, lanes, vals, 4);
    axis5_w9_fold_segment(&s1, lanes + 4, vals + 4, 4);
    axis5_w9_fold_segment(&s2, lanes + 8, vals + 8, 4);

    axis5_w9_encode_checkpoint(&s0, w0);
    axis5_w9_encode_checkpoint(&s1, w1);
    axis5_w9_encode_checkpoint(&s2, w2);
    axis5_w9_decode_checkpoint(&d0, w0);
    axis5_w9_decode_checkpoint(&d1, w1);
    axis5_w9_decode_checkpoint(&d2, w2);

    merged = d0;
    axis5_w9_merge(&merged, &d1);
    axis5_w9_merge(&merged, &d2);

    const unsigned int sig_full = axis5_w9_signature(&full);
    const unsigned int sig_merged = axis5_w9_signature(&merged);
    const unsigned int same = (sig_full == sig_merged) ? 1u : 0u;

    printf("%u %u %u\n", sig_full, sig_merged, same);
    return 0;
}
