struct ld_pair {
    long double first;
    long double second;
};

struct ld_pair ld_pair_make(long double a, long double b) {
    struct ld_pair pair;
    pair.first = a;
    pair.second = b;
    return pair;
}

long double ld_pair_second(struct ld_pair pair) {
    return pair.second;
}
