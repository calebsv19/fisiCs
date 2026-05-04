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

struct Rack {
    union Payload rows[2][2][2];
    int tail;
};

static unsigned checksum(const struct Rack *racks, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const union Payload *slot = &racks[i].rows[x][y][z];
                    acc = acc * 59u +
                          (unsigned char)slot->text[0] * 11u +
                          (unsigned char)slot->text[1] * 7u +
                          (unsigned char)slot->text[2] * 5u +
                          (unsigned)racks[i].tail;
                }
            }
        }
    }
    return acc;
}

int main(void) {
    struct Rack racks[2] = {
        [0] = {
            .rows[0] = {
                [1] = {
                    [0].text = "ax",
                    [1].text = "by",
                },
            },
            .rows[0][1][0].chars.c = 'r',
            .rows[0][1][1].chars.b = 'Q',
            .tail = 4,
        },
        [1] = {
            .rows[1] = {
                [0] = {
                    [0].text = "hi",
                    [1].text = "jk",
                },
            },
            .rows[1][0][0].chars.a = 'Z',
            .rows[1][0][1].text = "mn",
            .rows[1][0][1].chars.c = '!',
            .tail = 6,
        },
    };

    printf("%d %d %d %u\n",
           racks[0].rows[0][1][0].text[2],
           racks[0].rows[0][1][1].text[1],
           racks[1].rows[1][0][0].text[0],
           checksum(racks, 2));
    return 0;
}
