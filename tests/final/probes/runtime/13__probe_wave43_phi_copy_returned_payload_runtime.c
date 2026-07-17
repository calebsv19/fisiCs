#include <stdio.h>

typedef union {
    struct {
        int left;
        int right;
        int scale;
    } pair;
    int raw[3];
} Wave43PhiPayload;

typedef struct {
    int kind;
    Wave43PhiPayload payload;
    int stamp;
} Wave43PhiBox;

static Wave43PhiBox make_pair(int seed) {
    Wave43PhiBox b = {1, {.pair = {seed + 1, seed + 4, seed % 5 + 2}}, seed + 30};
    return b;
}

static Wave43PhiBox make_raw(int seed) {
    Wave43PhiBox b = {2, {.raw = {seed * 2, seed * 3 + 1, seed - 5}}, seed + 50};
    return b;
}

static Wave43PhiBox select_box(int seed, int flip) {
    Wave43PhiBox a = make_pair(seed + 2);
    Wave43PhiBox b = make_raw(seed + 7);
    if (flip) {
        return b;
    }
    return a;
}

static int box_score(Wave43PhiBox b) {
    if (b.kind == 1) {
        return b.payload.pair.left * b.payload.pair.scale -
               b.payload.pair.right + b.stamp;
    }
    return b.payload.raw[0] + b.payload.raw[1] * 2 - b.payload.raw[2] + b.stamp;
}

int main(void) {
    Wave43PhiBox box = make_pair(3);
    int total = 0;
    int i;

    for (i = 0; i < 8; ++i) {
        Wave43PhiBox chosen = select_box(i + box.kind, (box_score(box) + i) & 1);
        Wave43PhiBox fallback = (Wave43PhiBox){1, {.pair = {i + 5, box.stamp - i, 3}}, i + 60};
        Wave43PhiBox merged = ((box_score(chosen) + i) > box_score(box)) ? chosen : fallback;

        if ((i % 4) == 3) {
            merged = box;
            merged.stamp += i + merged.kind;
        }

        box = merged;
        total += box_score(box);
    }

    printf("%d %d %d %d\n", box.kind, box.stamp, box_score(box), total);
    return 0;
}
