#include <stdio.h>

typedef struct Cell {
    int x;
    double y;
} Cell;

static long long mix(Cell a,
                     int i0,
                     double d0,
                     long long l0,
                     Cell b,
                     unsigned u0,
                     double d1,
                     int i1,
                     Cell c,
                     long long l1) {
    long long acc = 0;
    acc += (long long)a.x + (long long)(a.y * 10.0);
    acc += (long long)i0 * 3 + (long long)(d0 * 10.0) + l0;
    acc += (long long)b.x * 5 + (long long)(b.y * 10.0);
    acc += (long long)u0 + (long long)i1 * 7 + (long long)(d1 * 10.0);
    acc += (long long)c.x * 11 + (long long)(c.y * 10.0) + l1 * 13;
    return acc;
}

int main(void) {
    Cell a = {2, 1.5};
    Cell b = {4, 2.25};
    Cell c = {6, 3.75};
    long long out = mix(a, 7, 4.5, 11, b, 13u, 5.25, 17, c, 19);

    printf("%lld\n", out);
    return 0;
}
