#include <stddef.h>
#include <stdio.h>

union Word {
    unsigned char bytes[4];
    struct {
        unsigned char a;
        unsigned char b;
        unsigned char c;
        unsigned char d;
    } parts;
};

struct Block {
    union Word words[2];
    unsigned char mark;
};

struct Image {
    struct Block blocks[2];
};

static unsigned checksum(const struct Image *image) {
    unsigned acc = 0;

    for (int b = 0; b < 2; ++b) {
        acc = acc * 47u + image->blocks[b].mark;
        for (int w = 0; w < 2; ++w) {
            const union Word *word = &image->blocks[b].words[w];
            acc = acc * 31u + word->bytes[0];
            acc = acc * 23u + word->bytes[1];
            acc = acc * 17u + word->bytes[3];
        }
    }

    return acc;
}

int main(void) {
    struct Image image = {
        .blocks[0].words[0].bytes = { 1, 2, 3, 4 },
        .blocks[0].words[0].parts.b = 9,
        .blocks[0].words[1].parts = { .a = 5, .d = 7 },
        .blocks[0].mark = 11,
        .blocks[1] = {
            .words = {
                [0].parts = { .b = 13, .c = 17 },
                [1].bytes = { 19, 23, 29, 31 },
            },
            .mark = 37,
        },
        .blocks[1].words[1].parts.c = 41,
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Image, blocks),
           (unsigned)offsetof(struct Block, mark),
           (unsigned)image.blocks[0].words[0].bytes[1],
           (unsigned)image.blocks[0].words[1].bytes[2],
           (unsigned)image.blocks[1].words[1].bytes[2],
           checksum(&image));
    return 0;
}
