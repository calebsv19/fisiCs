struct Bucket10Pair {
    int left;
    int right;
};

struct Bucket10Pair bucket10_pair;

void bucket10_pair_seed(int left, int right) {
    bucket10_pair.left = left;
    bucket10_pair.right = right;
}

int bucket10_pair_adjust_left(int delta) {
    bucket10_pair.left += delta;
    return bucket10_pair.left;
}
