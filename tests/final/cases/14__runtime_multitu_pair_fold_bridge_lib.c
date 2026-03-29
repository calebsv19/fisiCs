struct Pair {
    int a;
    int b;
};

struct Pair step_pair(struct Pair p, int seed) {
    p.a = (p.a * 3 + seed + p.b) % 1019;
    p.b = (p.b * 5 + seed * 2 + p.a) % 1019;
    return p;
}

int pair_score(struct Pair p) {
    return p.a * 2 - p.b + 17;
}
