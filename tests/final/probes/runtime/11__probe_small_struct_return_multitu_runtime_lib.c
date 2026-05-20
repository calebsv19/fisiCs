typedef struct ProbePair {
    int left;
    int right;
} ProbePair;

ProbePair probe_make_pair(int seed) {
    ProbePair pair;
    pair.left = seed;
    pair.right = seed + 3;
    return pair;
}
