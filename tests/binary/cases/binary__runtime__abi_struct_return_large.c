#include <stdio.h>

typedef struct {
    long long a;
    int b;
    int c;
    long long d;
} WidePair;

static WidePair make_wide_pair(int seed) {
    WidePair value;
    value.a = (long long)seed * 1000000000LL + 7LL;
    value.b = seed + 1;
    value.c = seed + 2;
    value.d = (long long)seed * 2000000000LL + 9LL;
    return value;
}

static long long score(WidePair value) {
    long long left = value.a / 1000000000LL;
    long long right = value.d / 2000000000LL;
    return left * 100LL + (long long)value.b * 10LL + (long long)value.c + right;
}

int main(void) {
    WidePair pair = make_wide_pair(4);
    printf("%lld\n", score(pair));
    return 0;
}
