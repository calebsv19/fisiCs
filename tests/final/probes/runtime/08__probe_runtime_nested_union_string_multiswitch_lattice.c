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

struct Rack {
    union Payload slots[2][2];
    int tag;
};

static unsigned checksum(const struct Rack *racks, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                const union Payload *slot = &racks[i].slots[r][c];
                acc = acc * 47u +
                      (unsigned char)slot->text[0] * 11u +
                      (unsigned char)slot->text[1] * 7u +
                      (unsigned char)slot->text[2] * 5u +
                      (unsigned char)slot->text[3] * 3u +
                      (unsigned)racks[i].tag;
            }
        }
    }
    return acc;
}

int main(void) {
    struct Rack racks[2] = {
        [0] = {
            .slots[0][0].text = "flare",
            .slots[0][0].chars.g = '?',
            .slots[0][0].chars.b = 'x',
            .slots[0][1].chars.a = 'm',
            .slots[0][1].text = "io",
            .slots[0][1].chars.d = '!',
            .tag = 2,
        },
        [1] = {
            .slots[1][0].text = "ax",
            .slots[1][0].chars.c = 'r',
            .slots[1][0].chars.h = '#',
            .slots[1][1].chars.a = 'q',
            .slots[1][1].chars.c = 'n',
            .slots[1][1].text = "uv",
            .slots[1][1].chars.b = 'p',
            .tag = 5,
        },
    };

    printf("%d %d %d %u\n",
           racks[0].slots[0][0].text[1],
           racks[0].slots[0][1].text[3],
           racks[1].slots[1][1].text[1],
           checksum(racks, 2));
    return 0;
}
