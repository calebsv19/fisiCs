#include <stdio.h>

struct Pair {
    int a;
    long long b;
};

static struct Pair make_left(int k) {
    struct Pair p;
    p.a = k + 1;
    p.b = (long long)k * 3 + 7;
    return p;
}

static struct Pair make_right(int k) {
    struct Pair p;
    p.a = k + 2;
    p.b = (long long)k * 5 + 11;
    return p;
}

int main(void) {
    long long sum = 0;
    int mix = 0;

    for (int i = 0; i < 8; ++i) {
        struct Pair p = (i % 3) ? make_left(i) : make_right(i + 1);
        sum += p.b;
        mix ^= (p.a + i);
    }

    printf("%lld %d\n", sum, mix);
    return 0;
}

