#include <stdio.h>

typedef struct {
    int a;
    long long b;
    double c;
} Mixed;

static Mixed build(int x) {
    Mixed m = {x, (long long)x * 1000000000LL + 7LL, x * 0.5};
    return m;
}

static long long score(Mixed m) {
    return (long long)m.a + m.b + (long long)(m.c * 10.0);
}

int main(void) {
    Mixed value = build(3);
    printf("%lld\n", score(value));
    return 0;
}
