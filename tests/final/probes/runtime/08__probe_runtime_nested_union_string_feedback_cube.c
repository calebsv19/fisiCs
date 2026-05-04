#include <stdio.h>

union Payload {
    char text[8];
    struct {
        char a;
        char b;
        char c;
        char d;
        char e;
        char f;
        char g;
        char h;
    } chars;
};

struct Grid {
    union Payload slots[2][2];
    int tag;
};

static unsigned checksum(const struct Grid *grids, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                const union Payload *slot = &grids[i].slots[r][c];
                acc = acc * 31u +
                      (unsigned char)slot->text[0] * 13u +
                      (unsigned char)slot->text[1] * 7u +
                      (unsigned char)slot->text[2] * 5u +
                      (unsigned char)slot->text[3] * 3u +
                      (unsigned)grids[i].tag;
            }
        }
    }
    return acc;
}

int main(void) {
    struct Grid grids[2] = {
        [0] = {
            .slots[0][0].text = "beam",
            .slots[0][0].chars.c = 'x',
            .slots[0][1].chars.a = 'k',
            .slots[0][1].text = "io",
            .slots[1][0].text = "sun",
            .slots[1][0].chars.d = '!',
            .tag = 3,
        },
        [1] = {
            .slots[0][1].text = "ax",
            .slots[0][1].chars.b = 'p',
            .slots[1][0].chars.a = 'm',
            .slots[1][0].chars.c = 'n',
            .slots[1][0].text = "uv",
            .slots[1][1].text = "qr",
            .slots[1][1].chars.h = '#',
            .tag = 6,
        },
    };

    printf("%d %d %d %u\n",
           grids[0].slots[0][0].text[2],
           grids[0].slots[1][0].text[3],
           grids[1].slots[1][0].text[1],
           checksum(grids, 2));
    return 0;
}
