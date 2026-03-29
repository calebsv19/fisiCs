#include <stdio.h>

typedef struct Lane {
    int i;
    double d;
} Lane;

static long long frontier(
    int a0, long long b0, double c0, Lane l0,
    int a1, long long b1, double c1, Lane l1,
    int a2, long long b2, double c2, Lane l2,
    int a3, long long b3, double c3, Lane l3
) {
    long long acc = 0;
    acc += (long long)a0 * 3 + b0 + (long long)(c0 * 10.0);
    acc += (long long)l0.i * 5 + (long long)(l0.d * 10.0);
    acc += (long long)a1 * 7 + b1 + (long long)(c1 * 10.0);
    acc += (long long)l1.i * 11 + (long long)(l1.d * 10.0);
    acc += (long long)a2 * 13 + b2 + (long long)(c2 * 10.0);
    acc += (long long)l2.i * 17 + (long long)(l2.d * 10.0);
    acc += (long long)a3 * 19 + b3 + (long long)(c3 * 10.0);
    acc += (long long)l3.i * 23 + (long long)(l3.d * 10.0);
    return acc;
}

int main(void) {
    Lane l0 = {2, 1.25};
    Lane l1 = {4, 2.5};
    Lane l2 = {6, 3.75};
    Lane l3 = {8, 4.5};

    long long out = frontier(
        3, 101LL, 0.5, l0,
        5, 202LL, 1.5, l1,
        7, 303LL, 2.5, l2,
        9, 404LL, 3.5, l3
    );

    printf("%lld\n", out);
    return 0;
}
