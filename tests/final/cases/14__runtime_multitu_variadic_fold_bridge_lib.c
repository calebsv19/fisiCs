typedef struct Pair {
    int a;
    long long b;
} Pair;

Pair make_pair(int seed) {
    Pair p;
    p.a = seed * 3 + 1;
    p.b = (long long)seed * 10007LL - 33LL;
    return p;
}

long long pair_score(Pair p) {
    return (long long)p.a * 1000LL + p.b;
}

long long fold4(long long a, long long b, long long c, long long d) {
    long long acc = 0;
    acc = acc * 7LL + a * 3LL;
    acc = acc * 7LL + b * 4LL;
    acc = acc * 7LL + c * 5LL;
    acc = acc * 7LL + d * 6LL;
    return acc;
}
