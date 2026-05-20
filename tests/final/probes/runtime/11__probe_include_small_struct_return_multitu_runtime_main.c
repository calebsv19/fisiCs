#include <stdio.h>

typedef struct ProbePairInc {
    int left;
    int right;
} ProbePairInc;

ProbePairInc probe_make_pair_inc(int seed);

int main(void) {
    ProbePairInc pair = probe_make_pair_inc(4);
    printf("%d %d\n", pair.left, pair.right);
    return (pair.left == 8 && pair.right == 9) ? 0 : 1;
}
