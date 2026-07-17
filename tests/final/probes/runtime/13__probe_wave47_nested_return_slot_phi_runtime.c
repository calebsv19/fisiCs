#include <stdio.h>

typedef struct {
    int slot[2];
    int weight;
} Wave47Inner;

typedef struct {
    Wave47Inner items[2];
    int stamp;
} Wave47Outer;

static Wave47Inner make_inner(int seed, int twist) {
    Wave47Inner inner;
    inner.slot[0] = seed * 3 + twist;
    inner.slot[1] = seed - twist * 2;
    inner.weight = seed * seed - twist;
    return inner;
}

static Wave47Outer make_outer(int seed, int twist) {
    Wave47Outer outer;
    outer.items[0] = make_inner(seed + 1, twist);
    outer.items[1] = make_inner(seed + 2, twist + 1);
    outer.stamp = seed * 9 + twist;
    return outer;
}

static Wave47Outer merge_outer(Wave47Outer a, Wave47Outer b, int select) {
    Wave47Outer out = select ? a : b;
    out.items[select & 1].slot[0] += b.items[0].weight - a.items[1].slot[1];
    out.items[(select + 1) & 1].slot[1] -= a.stamp - b.stamp + select;
    out.stamp += out.items[0].slot[0] - out.items[1].slot[1];
    return out;
}

static int checksum(Wave47Outer outer) {
    return outer.stamp
        + outer.items[0].slot[0] * 2
        - outer.items[0].slot[1] * 3
        + outer.items[0].weight * 5
        - outer.items[1].slot[0] * 7
        + outer.items[1].slot[1] * 11
        - outer.items[1].weight * 13;
}

int main(void) {
    Wave47Outer current = make_outer(2, 1);
    int total = 0;
    int i;

    for (i = 0; i < 9; ++i) {
        Wave47Outer left = make_outer(i + 3, current.stamp & 7);
        Wave47Outer right = merge_outer(current, left, i & 1);
        current = ((checksum(left) + total) & 3) ? merge_outer(left, right, i) : merge_outer(right, current, i + 1);
        total += checksum(current);
    }

    printf("%d %d %d %d\n", current.stamp, current.items[0].slot[0], checksum(current), total);
    return 0;
}
