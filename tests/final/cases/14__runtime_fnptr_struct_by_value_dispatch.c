#include <stdio.h>

typedef struct {
    int x;
    int y;
} Pair;

typedef Pair (*PairOp)(Pair, int);

static Pair op_add(Pair p, int k) {
    p.x += k;
    p.y -= k;
    return p;
}

static Pair op_mix(Pair p, int k) {
    int ox = p.x;
    p.x = p.x * k + p.y;
    p.y = p.y * k - ox;
    return p;
}

int main(void) {
    Pair items[3] = {{2, 5}, {3, 7}, {4, 9}};
    PairOp ops[2] = {op_add, op_mix};
    long long accx = 0;
    long long accy = 0;

    for (int i = 0; i < 3; ++i) {
        Pair out = ops[i & 1](items[i], i + 1);
        accx += out.x;
        accy += out.y;
        items[i] = out;
    }

    int checksum = items[0].x + items[1].y + items[2].x;
    printf("%lld %lld %d\n", accx, accy, checksum);
    return 0;
}
