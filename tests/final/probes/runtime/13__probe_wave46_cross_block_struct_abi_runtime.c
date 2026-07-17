#include <stdio.h>

typedef struct {
    int a;
    int b;
    int c;
    int d;
} Wave46Quad;

static Wave46Quad make_quad(int seed) {
    Wave46Quad quad = {seed + 1, seed * 2 + 3, seed * 3 - 5, seed * 4 + 7};
    return quad;
}

static Wave46Quad rotate_quad(Wave46Quad input, int step) {
    Wave46Quad out;
    out.a = input.b + step;
    out.b = input.c - step;
    out.c = input.d + input.a;
    out.d = input.a - input.b + step * 2;
    return out;
}

static int fold_quad(Wave46Quad quad) {
    return quad.a * 2 - quad.b * 3 + quad.c * 5 - quad.d * 7;
}

static int consume_quad(Wave46Quad (*maker)(int), int seed, int limit) {
    Wave46Quad quad = maker(seed);
    int total = 0;
    int i;
    for (i = 0; i < limit; ++i) {
        Wave46Quad next = rotate_quad(quad, i + seed);
        if ((fold_quad(next) + i) > fold_quad(quad)) {
            quad = next;
        } else {
            quad.a += i;
            quad.d -= seed;
        }
        total += fold_quad(quad);
    }
    return total + fold_quad(quad);
}

int main(void) {
    int left = consume_quad(make_quad, 3, 7);
    int right = consume_quad(make_quad, 6, 5);
    printf("%d %d %d\n", left, right, left - right);
    return 0;
}
