struct Pair {
    int a;
    long long b;
};

static struct Pair tweak(struct Pair p, int d) {
    p.a += d;
    p.b += (long long)d * 10;
    return p;
}

int main(void) {
    struct Pair p = {2, 5};
    struct Pair q = tweak(p, 3);
    return q.a + (int)q.b;
}
