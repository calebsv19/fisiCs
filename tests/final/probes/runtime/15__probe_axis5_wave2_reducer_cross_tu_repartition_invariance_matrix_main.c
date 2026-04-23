#include <stdio.h>

typedef struct Axis5W2Accum {
    unsigned int sum0;
    unsigned int sum1;
    unsigned int xormix;
} Axis5W2Accum;

void axis5_w2_accum_update(Axis5W2Accum* a, unsigned int lane, unsigned int value);
void axis5_w2_accum_fold_segment(
    Axis5W2Accum* out,
    const unsigned int* lanes,
    const unsigned int* values,
    int n
);
void axis5_w2_accum_merge(Axis5W2Accum* dst, const Axis5W2Accum* src);
unsigned int axis5_w2_accum_signature(const Axis5W2Accum* a);

int main(void) {
    const unsigned int lanes[] = {0u, 1u, 2u, 3u, 2u, 1u, 0u, 3u, 1u};
    const unsigned int vals[] = {7u, 4u, 9u, 6u, 2u, 8u, 5u, 3u, 10u};

    Axis5W2Accum full = {0u, 0u, 0u};
    Axis5W2Accum p0, p1, p2;
    Axis5W2Accum merged = {0u, 0u, 0u};

    for (int i = 0; i < 9; ++i) axis5_w2_accum_update(&full, lanes[i], vals[i]);

    axis5_w2_accum_fold_segment(&p0, lanes, vals, 3);
    axis5_w2_accum_fold_segment(&p1, lanes + 3, vals + 3, 3);
    axis5_w2_accum_fold_segment(&p2, lanes + 6, vals + 6, 3);

    axis5_w2_accum_merge(&merged, &p0);
    axis5_w2_accum_merge(&merged, &p1);
    axis5_w2_accum_merge(&merged, &p2);

    const unsigned int sig_full = axis5_w2_accum_signature(&full);
    const unsigned int sig_part = axis5_w2_accum_signature(&merged);
    const unsigned int same = (sig_full == sig_part) ? 1u : 0u;

    printf("%u %u %u\n", sig_full, sig_part, same);
    return 0;
}
