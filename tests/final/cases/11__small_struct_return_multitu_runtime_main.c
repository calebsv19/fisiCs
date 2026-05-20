#include <stdio.h>

typedef struct ProbePair {
    int left;
    int right;
} ProbePair;

ProbePair probe_make_pair(int seed);

int main(void) {
    ProbePair pair = probe_make_pair(9);
    if (pair.left != 9) return 1;
    if (pair.right != 12) return 2;
    printf("%d %d\n", pair.left, pair.right);
    return 0;
}
