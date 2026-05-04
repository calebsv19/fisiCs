#include <stdio.h>

union Payload {
    char text[6];
    struct {
        char a;
        char b;
        char c;
        char d;
        char e;
        char f;
    } chars;
};

struct Node {
    int tag;
    union Payload payload;
    int tail;
};

static unsigned checksum(const struct Node *nodes, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        acc = acc * 29u +
              (unsigned char)nodes[i].payload.text[0] * 7u +
              (unsigned char)nodes[i].payload.text[1] * 5u +
              (unsigned char)nodes[i].payload.text[2] * 3u +
              (unsigned)nodes[i].tag * 11u +
              (unsigned)nodes[i].tail;
    }
    return acc;
}

int main(void) {
    struct Node nodes[3] = {
        [0] = {
            .payload.text = "go",
            .tail = 1,
        },
        [1] = {
            .tag = 2,
            .payload.chars = {
                .a = 'x',
                .b = 'y',
                .d = 'z',
            },
        },
        [2] = {
            .payload.text = "cat",
            .tail = 4,
        },
    };

    printf("%d %d %d %u\n",
           nodes[0].payload.text[2],
           nodes[1].payload.text[2],
           nodes[2].payload.text[1],
           checksum(nodes, 3));
    return 0;
}
