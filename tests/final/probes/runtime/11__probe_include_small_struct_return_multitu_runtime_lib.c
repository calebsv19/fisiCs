typedef struct ProbePairInc {
    int left;
    int right;
} ProbePairInc;

ProbePairInc probe_make_pair_inc(int seed) {
    ProbePairInc pair;
    pair.left = seed * 2;
    pair.right = pair.left + 1;
    return pair;
}
