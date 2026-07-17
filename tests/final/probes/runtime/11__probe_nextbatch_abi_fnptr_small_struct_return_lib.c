struct NextAbiPair {
    int x;
    int y;
};

typedef struct NextAbiPair (*NextAbiPairFn)(int, int);

static struct NextAbiPair nextbatch_pair_add(int a, int b) {
    struct NextAbiPair out;
    out.x = a + b + 3;
    out.y = a * 2 - b + 5;
    return out;
}

static struct NextAbiPair nextbatch_pair_mix(int a, int b) {
    struct NextAbiPair out;
    out.x = a * 4 + b - 9;
    out.y = b * 3 - a + 11;
    return out;
}

NextAbiPairFn nextbatch_pick_pair_fn(int selector) {
    NextAbiPairFn table[2];
    table[0] = nextbatch_pair_add;
    table[1] = nextbatch_pair_mix;
    return table[(unsigned)selector & 1u];
}

int nextbatch_pair_fold(struct NextAbiPair pair, int salt) {
    return pair.x * 17 + pair.y * 31 + salt;
}
