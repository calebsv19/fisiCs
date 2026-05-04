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
            acc = acc * 37u +
                  (unsigned char)rows[i].slots[j].text[0] * 11u +
                  (unsigned char)rows[i].slots[j].text[1] * 7u +
                  (unsigned char)rows[i].slots[j].text[2] * 5u +
                  (unsigned char)rows[i].slots[j].text[3] * 3u +
                  (unsigned)rows[i].tag;
        }
    }
    return acc;
}

int main(void) {
    struct Row rows[2] = {
        [0] = {
            .slots[0].text = "sun",
            .slots[0].chars.d = '!',
            .slots[1].text = "io",
            .slots[1].chars.b = 'q',
            .tag = 3,
        },
        [1] = {
            .slots[0].text = "ax",
            .slots[0].chars.c = 'r',
            .slots[1].chars.a = 'm',
            .slots[1].chars.c = 'n',
            .tag = 5,
        },
    };

    printf("%d %d %d %u\n",
           rows[0].slots[0].text[3],
           rows[0].slots[1].text[1],
           rows[1].slots[0].text[2],
           checksum(rows, 2));
    return 0;
}
