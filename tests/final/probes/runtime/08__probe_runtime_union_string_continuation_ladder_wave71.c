#include <stdio.h>

union Payload {
    char text[6];
    struct {
        unsigned char a;
        unsigned char b;
        unsigned char c;
        unsigned char d;
        unsigned char e;
        unsigned char f;
    } bytes;
};

struct Row {
    union Payload payload;
    int mark;
};

struct Grid {
    struct Row rows[2][2];
    int tail;
};

static unsigned checksum(const struct Grid grids[2]) {
    unsigned acc = 0;

    for (int g = 0; g < 2; ++g) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                const struct Row *row = &grids[g].rows[r][c];
                acc = acc * 37u +
                      row->payload.bytes.a * 11u +
                      row->payload.bytes.b * 7u +
                      row->payload.bytes.c * 5u +
                      row->payload.bytes.d * 3u +
                      row->payload.bytes.e +
                      row->payload.bytes.f * 13u +
                      (unsigned)row->mark * 17u +
                      (unsigned)grids[g].tail;
            }
        }
    }

    return acc;
}

int main(void) {
    struct Grid grids[2] = {
        [0] = {
            .rows[0][0].payload = {
                .text = { 'h', 'i', 0, '!', 0, 0 },
            },
            .rows[0][0].mark = 2,
            .rows[1][1].payload = {
                .text = { 'u', 'v', 0, 0, 0, '?' },
            },
            .rows[1][1].mark = 5,
            .tail = 4,
        },
        [1] = {
            .rows[0][1].payload = {
                .text = { 'c', 'a', 't', 0, 'Z', 0 },
            },
            .rows[0][1].mark = 7,
            .rows[1][0].payload = {
                .text = { 'n', 'o', 0, 'm', 0, 0 },
            },
            .rows[1][0].mark = 3,
            .tail = 6,
        },
    };

    printf("%u %u %u %u\n",
           (unsigned)grids[0].rows[0][0].payload.bytes.d,
           (unsigned)grids[0].rows[1][1].payload.bytes.f,
           (unsigned)grids[1].rows[0][1].payload.bytes.e,
           checksum(grids));
    return 0;
}
