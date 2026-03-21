#include <stdio.h>

struct Pair {
    int x;
    int y;
};

typedef int (*pair_op_t)(const struct Pair*, int);

static int dot_like(const struct Pair* p, int scale) {
    return (p->x * scale) + p->y;
}

static int mix_like(const struct Pair* p, int scale) {
    return (p->y * scale) - p->x;
}

int main(void) {
    struct Pair data[3] = {{2, 5}, {3, 7}, {4, 11}};
    pair_op_t ops[2] = {dot_like, mix_like};
    int total = 0;
    int i = 0;

    for (i = 0; i < 3; ++i) {
        pair_op_t op = (i & 1) ? ops[1] : ops[0];
        total += op(&data[i], i + 2);
    }

    printf("%d\n", total);
    return 0;
}
