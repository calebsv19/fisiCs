#include <stdio.h>

typedef struct {
    int a;
    int b;
} Wave41Pair;

typedef union {
    struct {
        int row[2];
        Wave41Pair pair;
    } grid;
    struct {
        Wave41Pair pair[2];
    } pairs;
} Wave41Inner;

typedef struct {
    int tag;
    Wave41Inner inner;
    int checksum;
} Wave41Box;

static Wave41Box make_grid(int seed) {
    Wave41Box b;
    b.tag = 10;
    b.inner.grid.row[0] = seed + 1;
    b.inner.grid.row[1] = seed + 4;
    b.inner.grid.pair.a = seed * 2;
    b.inner.grid.pair.b = seed * 3;
    b.checksum = seed + 100;
    return b;
}

static Wave41Box make_pairs(int seed) {
    Wave41Box b;
    b.tag = 20;
    b.inner.pairs.pair[0].a = seed - 1;
    b.inner.pairs.pair[0].b = seed + 2;
    b.inner.pairs.pair[1].a = seed * 4;
    b.inner.pairs.pair[1].b = seed * 5;
    b.checksum = seed + 200;
    return b;
}

static int box_score(Wave41Box b) {
    if (b.tag == 10) {
        return b.inner.grid.row[0] * 2 + b.inner.grid.row[1] * 3 +
               b.inner.grid.pair.a - b.inner.grid.pair.b + b.checksum;
    }
    return b.inner.pairs.pair[0].a + b.inner.pairs.pair[0].b * 2 +
           b.inner.pairs.pair[1].a * 3 - b.inner.pairs.pair[1].b +
           b.checksum;
}

int main(void) {
    Wave41Box kept = make_grid(3);
    int total = 0;
    int i;

    for (i = 0; i < 5; ++i) {
        Wave41Box left = make_grid(i + 4);
        Wave41Box right = make_pairs(i + 7);
        Wave41Box selected = ((box_score(kept) + i) & 1) ? left : right;
        Wave41Box copied = (box_score(selected) > box_score(kept)) ? selected : kept;

        if (copied.tag == 10) {
            copied.inner.grid.pair.a += i;
        } else {
            copied.inner.pairs.pair[i & 1].b += copied.tag - i;
        }

        kept = copied;
        total += box_score(kept);
    }

    printf("%d %d %d %d\n", kept.tag, kept.checksum, box_score(kept), total);
    return 0;
}
