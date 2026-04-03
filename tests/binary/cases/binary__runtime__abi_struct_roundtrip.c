#include <stdio.h>

typedef struct {
    int a;
    int b;
    int c;
    int d;
} Quad;

static Quad tweak(Quad value, int delta) {
    value.a += delta;
    value.b -= delta;
    value.c *= 2;
    value.d /= 2;
    return value;
}

int main(void) {
    Quad q;
    q.a = 10;
    q.b = 20;
    q.c = 30;
    q.d = 40;
    q = tweak(q, 3);
    printf("%d\n", q.a + q.b + q.c + q.d);
    return 0;
}
