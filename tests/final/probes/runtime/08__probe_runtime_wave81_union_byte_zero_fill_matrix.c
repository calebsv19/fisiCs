#include <stddef.h>
#include <stdio.h>

union Bytes {
    unsigned char raw[5];
    struct {
        unsigned char first;
        unsigned char second;
        unsigned char third;
        unsigned char fourth;
        unsigned char fifth;
    } named;
};

struct Cell {
    unsigned char key;
    union Bytes bytes;
};

struct Matrix {
    struct Cell cells[2][2];
    unsigned char done;
};

static const struct Matrix matrix = {
    .cells[0][0].bytes.raw = { [0] = 3, [4] = 5 },
    .cells[0][0].key = 7,
    .cells[0][1] = { .key = 11, .bytes.named = { .second = 13, .fourth = 17 } },
    .cells[1][1].bytes.raw = { [2] = 19 },
    .done = 23,
};

int main(void) {
    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Matrix, done),
           (unsigned)matrix.cells[0][0].bytes.named.third,
           (unsigned)matrix.cells[0][0].bytes.raw[4],
           (unsigned)matrix.cells[0][1].bytes.raw[1],
           (unsigned)matrix.cells[1][1].bytes.named.third,
           (unsigned)matrix.cells[1][0].key);
    return 0;
}
