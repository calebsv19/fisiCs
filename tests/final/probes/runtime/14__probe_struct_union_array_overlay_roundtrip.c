#include <stdio.h>

typedef struct Outer {
    int tag;
    union {
        int i;
        double d;
    } u;
    int tail[2];
} Outer;

static Outer step(Outer in, int k) {
    Outer out = in;
    out.tag = in.tag + k;
    out.u.i = in.u.i + k * 10;
    out.tail[0] = in.tail[0] - k;
    out.tail[1] = in.tail[1] + k * 2;
    return out;
}

int main(void) {
    Outer a = {3, {0}, {20, 30}};
    a.u.i = 11;

    Outer b = step(a, 4);
    Outer c = step(b, -3);
    int mix = c.tag + c.u.i * 10 + c.tail[0] * 100 + c.tail[1] * 1000;

    printf("%d %d %d %d %d\n", c.tag, c.u.i, c.tail[0], c.tail[1], mix);
    return 0;
}
