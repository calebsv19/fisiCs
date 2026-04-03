#include <stdio.h>

typedef struct {
    int q;
    float r;
} Inner;

typedef struct {
    Inner a;
    Inner b;
} Outer;

static Outer build_outer(void) {
    Outer value;
    value.a.q = 5;
    value.a.r = 1.25f;
    value.b.q = 7;
    value.b.r = 2.5f;
    return value;
}

static int score_outer(Outer value) {
    return value.a.q + value.b.q +
           (int)(value.a.r * 4.0f) +
           (int)(value.b.r * 4.0f);
}

int main(void) {
    Outer value = build_outer();
    printf("%d\n", score_outer(value));
    return 0;
}
