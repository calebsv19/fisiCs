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

struct Cube {
    union Payload cells[2][2][2];
    int tags[2];
};

static unsigned checksum(const struct Cube *cubes, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const union Payload *cell = &cubes[i].cells[x][y][z];
                    acc = acc * 47u +
                          (unsigned char)cell->text[0] * 13u +
                          (unsigned char)cell->text[1] * 11u +
                          (unsigned char)cell->text[2] * 7u +
                          (unsigned char)cell->text[3] * 5u +
                          (unsigned)cubes[i].tags[z];
                }
            }
        }
    }
    return acc;
}

int main(void) {
    struct Cube cubes[2] = {
        [0] = {
            .cells[0][0][0].text = "ab",
            .cells[0][0][0].chars.d = 'Q',
            .cells[0][0][0].text = "uv",
            .cells[0][0][0].chars.b = 'x',
            .cells[1][0][1].chars.a = 'm',
            .cells[1][0][1].text = "io",
            .cells[1][0][1].chars.c = '!',
            .tags[0] = 2,
            .tags[1] = 5,
        },
        [1] = {
            .cells[0][1][0].text = "jk",
            .cells[0][1][0].chars.g = '?',
            .cells[0][1][0].text = "rs",
            .cells[0][1][0].chars.a = 'A',
            .cells[1][1][1].chars.a = 'q',
            .cells[1][1][1].chars.c = 'n',
            .cells[1][1][1].text = "lm",
            .cells[1][1][1].chars.b = 'p',
            .tags[0] = 3,
            .tags[1] = 7,
        },
    };

    printf("%d %d %d %u\n",
           cubes[0].cells[0][0][0].text[1],
           cubes[0].cells[1][0][1].text[2],
           cubes[1].cells[1][1][1].text[1],
           checksum(cubes, 2));
    return 0;
}
