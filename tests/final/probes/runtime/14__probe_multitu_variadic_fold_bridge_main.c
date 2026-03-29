#include <stdio.h>

typedef struct Pair {
    int a;
    long long b;
} Pair;

extern Pair make_pair(int seed);
extern long long pair_score(Pair p);
extern long long fold4(long long a, long long b, long long c, long long d);

int main(void) {
    Pair p0 = make_pair(9);
    Pair p1 = make_pair(14);
    long long x = pair_score(p0);
    long long y = pair_score(p1);
    long long z = fold4(x, y, x - y, y - x / 2);

    printf("%lld %lld %lld\n", x, y, z);
    return 0;
}
