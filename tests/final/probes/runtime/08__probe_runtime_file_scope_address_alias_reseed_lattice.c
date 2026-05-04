#include <stdio.h>

static int seed_a = 4;
static int seed_b = 7;
static int seed_c = 10;
static int seed_d = 12;

static int bump_one(int x) { return x + 1; }
static int bump_five(int x) { return x + 5; }

struct Slot {
    const int *source;
    int *mirror;
    int (*dispatch)(int);
};

struct Frame {
    struct Slot lanes[2][2];
    const int *tags[2];
};

static struct Frame frames[2] = {
    [0] = {
        .lanes[0][0].source = &seed_a,
        .lanes[0][0].mirror = &seed_b,
        .lanes[0][0].dispatch = bump_one,
        .lanes[0] = {
            [1] = {
                .source = &seed_c,
                .mirror = &seed_d,
                .dispatch = bump_five,
            },
        },
        .lanes[1][0] = {
            .source = &seed_b,
            .mirror = &seed_a,
            .dispatch = bump_one,
        },
        .tags[0] = &seed_d,
    },
    [1] = {
        .lanes[1][1].source = &seed_c,
        .lanes[1][1].mirror = &seed_b,
        .lanes[1][1].dispatch = bump_five,
        .lanes[1] = {
            [0] = {
                .source = &seed_d,
                .mirror = &seed_c,
                .dispatch = bump_one,
            },
        },
        .tags[0] = &seed_a,
        .tags[1] = &seed_b,
    },
};

static unsigned checksum(void) {
    unsigned acc = 0;

    for (int f = 0; f < 2; ++f) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                const struct Slot *slot = &frames[f].lanes[r][c];
                int src = slot->source ? *slot->source : 0;
                int mir = slot->mirror ? *slot->mirror : 0;
                int dsp = slot->dispatch ? slot->dispatch(f + r + c + 2) : 0;
                int tag = frames[f].tags[c] ? *frames[f].tags[c] : 0;
                acc = acc * 43u + (unsigned)(src * 11 + mir * 7 + dsp * 5 + tag);
            }
        }
    }
    return acc;
}

int main(void) {
    printf("%d %d %d %u\n",
           frames[0].lanes[0][1].dispatch ? frames[0].lanes[0][1].dispatch(3) : 0,
           frames[1].lanes[1][1].source ? *frames[1].lanes[1][1].source : 0,
           frames[1].tags[0] ? *frames[1].tags[0] : 0,
           checksum());
    return 0;
}
