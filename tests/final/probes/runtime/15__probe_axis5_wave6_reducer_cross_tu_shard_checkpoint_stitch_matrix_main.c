#include <stdio.h>

typedef struct Axis5W6Accum {
    unsigned int lane_sum[4];
    unsigned int xormix;
    unsigned int token;
} Axis5W6Accum;

void axis5_w6_accum_fold_segment(
    Axis5W6Accum* out,
    const unsigned int* lanes,
    const unsigned int* values,
    int n,
    unsigned int seed
);
void axis5_w6_accum_encode(const Axis5W6Accum* a, unsigned int wire[6]);
void axis5_w6_accum_decode(Axis5W6Accum* a, const unsigned int wire[6]);
void axis5_w6_accum_merge(Axis5W6Accum* dst, const Axis5W6Accum* src);
unsigned int axis5_w6_accum_signature(const Axis5W6Accum* a);

int main(void) {
    const unsigned int lanes[] = {0u, 1u, 3u, 2u, 1u, 0u, 2u, 3u, 0u, 2u, 1u, 3u};
    const unsigned int vals[] = {7u, 5u, 4u, 9u, 6u, 8u, 3u, 10u, 11u, 2u, 12u, 1u};

    Axis5W6Accum full;
    Axis5W6Accum s0;
    Axis5W6Accum s1;
    Axis5W6Accum s2;
    Axis5W6Accum d0;
    Axis5W6Accum d1;
    Axis5W6Accum d2;
    Axis5W6Accum merged;
    unsigned int w0[6];
    unsigned int w1[6];
    unsigned int w2[6];

    axis5_w6_accum_fold_segment(&full, lanes, vals, 12, 0u);
    axis5_w6_accum_fold_segment(&s0, lanes, vals, 4, 0u);
    axis5_w6_accum_fold_segment(&s1, lanes + 4, vals + 4, 4, 0u);
    axis5_w6_accum_fold_segment(&s2, lanes + 8, vals + 8, 4, 0u);

    axis5_w6_accum_encode(&s0, w0);
    axis5_w6_accum_encode(&s1, w1);
    axis5_w6_accum_encode(&s2, w2);
    axis5_w6_accum_decode(&d0, w0);
    axis5_w6_accum_decode(&d1, w1);
    axis5_w6_accum_decode(&d2, w2);

    merged = d0;
    axis5_w6_accum_merge(&merged, &d1);
    axis5_w6_accum_merge(&merged, &d2);

    const unsigned int sig_full = axis5_w6_accum_signature(&full);
    const unsigned int sig_merged = axis5_w6_accum_signature(&merged);
    const unsigned int same = (sig_full == sig_merged) ? 1u : 0u;

    printf("%u %u %u\n", sig_full, sig_merged, same);
    return 0;
}
