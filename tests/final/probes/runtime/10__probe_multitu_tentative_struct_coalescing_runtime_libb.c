struct Bucket10Pair {
    int left;
    int right;
};

struct Bucket10Pair bucket10_pair;

int bucket10_pair_adjust_right(int delta) {
    bucket10_pair.right += delta;
    return bucket10_pair.right;
}

int bucket10_pair_total(void) {
    return bucket10_pair.left + bucket10_pair.right;
}
