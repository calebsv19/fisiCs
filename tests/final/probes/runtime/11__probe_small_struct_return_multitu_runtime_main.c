#include <stdio.h>

typedef struct ProbePair {
    int left;
    int right;
} ProbePair;

ProbePair probe_make_pair(int seed);

int main(void) {
    ProbePair pair = probe_make_pair(9);
    printf("%d %d\n", pair.left, pair.right);
    return (pair.left == 9 && pair.right == 12) ? 0 : 1;
}
