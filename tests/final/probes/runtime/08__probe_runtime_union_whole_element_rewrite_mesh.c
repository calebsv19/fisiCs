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

struct Row {
    union Payload slots[2];
    int tag;
};

static unsigned checksum(const struct Row *rows, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < 2; ++j) {
            const union Payload *slot = &rows[i].slots[j];
            acc = acc * 41u +
                  (unsigned char)slot->text[0] * 11u +
                  (unsigned char)slot->text[1] * 7u +
                  (unsigned char)slot->text[2] * 5u +
                  (unsigned char)slot->text[3] * 3u +
                  (unsigned)rows[i].tag;
        }
    }
    return acc;
}

int main(void) {
    struct Row rows[2] = {
        [0] = {
            .slots[0].chars.c = 'x',
            .slots[0] = {
                .text = "sun",
            },
            .slots[1].text = "io",
            .slots[1] = {
                .chars = {
                    .a = 'm',
                    .d = '!',
                },
            },
            .tag = 3,
        },
        [1] = {
            .slots[0].text = "ax",
            .slots[0] = {
                .chars = {
                    .b = 'p',
                    .c = 'r',
                },
            },
            .slots[1].chars.a = 'q',
            .slots[1] = {
                .text = "uv",
            },
            .tag = 5,
        },
    };

    printf("%d %d %d %u\n",
           rows[0].slots[0].text[0],
           rows[0].slots[1].text[3],
           rows[1].slots[0].text[2],
           checksum(rows, 2));
    return 0;
}
