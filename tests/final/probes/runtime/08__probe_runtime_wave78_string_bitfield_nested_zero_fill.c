#include <stddef.h>
#include <stdio.h>

struct Bits {
    unsigned int a : 4;
    unsigned int b : 4;
};

struct Item {
    char label[5];
    struct Bits bits;
};

struct Shelf {
    struct Item items[2][2];
    unsigned char stamp;
};

static unsigned checksum(const struct Shelf *shelf) {
    unsigned acc = shelf->stamp;

    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            const struct Item *item = &shelf->items[r][c];
            acc = acc * 41u + (unsigned char)item->label[0];
            acc = acc * 31u + (unsigned char)item->label[4];
            acc = acc * 29u + item->bits.a;
            acc = acc * 23u + item->bits.b;
        }
    }

    return acc;
}

int main(void) {
    struct Shelf shelf = {
        .items[0][0] = { .label = "hi", .bits = { .a = 3 } },
        .items[0][1].bits.b = 7,
        .items[1] = {
            [0].label = { 'r', 'o', 'w', 0, 0 },
            [1] = { .label = "xy", .bits = { .a = 9, .b = 11 } },
        },
        .items[1][1].label[2] = 'z',
        .stamp = 13,
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Shelf, items),
           (unsigned)offsetof(struct Shelf, stamp),
           (unsigned)shelf.items[0][0].label[2],
           (unsigned)shelf.items[0][1].bits.a,
           (unsigned char)shelf.items[1][1].label[2],
           checksum(&shelf));
    return 0;
}
