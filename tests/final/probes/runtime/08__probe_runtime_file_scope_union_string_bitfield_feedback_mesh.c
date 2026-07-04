#include <stdio.h>

union Payload {
    char text[4];
    struct {
        unsigned char a;
        unsigned char b;
        unsigned char c;
        unsigned char d;
    } bytes;
};

struct Flags {
    unsigned lo : 3;
    unsigned hi : 5;
};

struct Row {
    union Payload payload;
    struct Flags flags;
    int mark;
};

struct Grid {
    struct Row rows[2][2];
    int tail;
};

static struct Grid grids[2] = {
    [0] = {
        .rows[0][1].payload.text = "ab",
        .rows[0][1].payload.text[2] = 'Q',
        .rows[0][1].flags.lo = 5,
        .rows[0][1].flags.hi = 17,
        .rows[0][1].mark = 3,
        .rows[1][0].payload.text = "xy",
        .rows[1][0].flags = { .lo = 2, .hi = 7 },
        .rows[1][0].mark = 4,
        .tail = 8,
    },
    [1] = {
        .rows[1][1].payload.bytes = { 'm', 'n', 'o', 0 },
        .rows[1][1].payload = { .text = "k" },
        .rows[1][1].payload.text[1] = '!',
        .rows[1][1].flags = { .lo = 1, .hi = 9 },
        .rows[1][1].mark = 6,
        .rows[0][0].payload.text = "rt",
        .rows[0][0].flags.lo = 4,
        .rows[0][0].flags.hi = 11,
        .rows[0][0].mark = 2,
        .tail = 5,
    },
};

static unsigned checksum(void) {
    unsigned acc = 0;

    for (int g = 0; g < 2; ++g) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                const struct Row *row = &grids[g].rows[r][c];
                acc = acc * 33u +
                      row->payload.bytes.a +
                      row->payload.bytes.b * 3u +
                      row->payload.bytes.c * 5u +
                      row->payload.bytes.d * 7u +
                      row->flags.lo * 11u +
                      row->flags.hi * 13u +
                      (unsigned)row->mark * 17u +
                      (unsigned)grids[g].tail;
            }
        }
    }

    return acc;
}

int main(void) {
    printf("%u %u %u %u\n",
           (unsigned char)grids[0].rows[0][1].payload.text[2],
           (unsigned char)grids[1].rows[1][1].payload.text[1],
           grids[1].rows[0][0].flags.hi,
           checksum());
    return 0;
}
