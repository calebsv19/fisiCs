#include <stdio.h>

typedef union {
    struct {
        int a;
        int b;
        int c;
    } triple;
    int raw[3];
} Wave44ShortPayload;

typedef struct {
    int tag;
    Wave44ShortPayload payload;
    int guard;
} Wave44ShortBox;

static int hits;

static int tick(int value) {
    hits += value;
    return (value & 1) != 0;
}

static Wave44ShortBox make_triple(int seed) {
    Wave44ShortBox box = {1, {.triple = {seed + 2, seed * 3, seed - 4}}, seed + 30};
    return box;
}

static Wave44ShortBox make_raw(int seed) {
    Wave44ShortBox box = {2, {.raw = {seed * 2 + 1, seed + 7, seed * 3 - 5}}, seed + 50};
    return box;
}

static int score(Wave44ShortBox box) {
    if (box.tag == 1) {
        return box.payload.triple.a * 5 - box.payload.triple.b +
               box.payload.triple.c * 3 + box.guard;
    }
    return box.payload.raw[0] - box.payload.raw[1] * 2 +
           box.payload.raw[2] + box.guard;
}

int main(void) {
    Wave44ShortBox current = make_triple(3);
    int total = 0;
    int i;

    for (i = 0; i < 9; ++i) {
        Wave44ShortBox left = make_triple(i + current.tag);
        Wave44ShortBox right = make_raw(i + current.guard % 7);
        Wave44ShortBox selected;

        if ((tick(i + current.tag) && score(left) > score(current)) ||
            (!tick(i + 2) && score(right) >= score(current) - i)) {
            selected = left;
            selected.guard += hits & 7;
        } else {
            selected = right;
            selected.payload.raw[i % 3] += current.guard - i;
        }

        current = (score(selected) & 1) ? selected : current;
        total += score(current) + hits;
    }

    printf("%d %d %d %d %d\n", current.tag, current.guard, score(current), total, hits);
    return 0;
}
