#include <stdio.h>

struct Pair {
    int a;
    int b;
};

union Mix {
    struct Pair pair;
    int scalar;
};

struct Node {
    int scale;
    union Mix mix;
};

static struct Node bump(struct Node in) {
    in.mix.pair.a += in.scale;
    in.mix.pair.b += in.scale * 2;
    return in;
}

int main(void) {
    struct Node n;
    n.scale = 4;
    n.mix.pair.a = 3;
    n.mix.pair.b = 8;

    struct Node out = bump(n);
    printf("%d %d\n", out.mix.pair.a, out.mix.pair.b);
    return 0;
}
