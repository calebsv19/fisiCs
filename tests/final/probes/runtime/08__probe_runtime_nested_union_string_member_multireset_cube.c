#include <stdio.h>

union Payload {
    char text[8];
    struct {
        char a,b,c,d,e,f,g,h;
    } chars;
};

struct Cell {
    union Payload payload;
    int mark;
};

struct CellCube {
    struct Cell cells[2][2][2];
};

static unsigned checksum(const struct Cell cells[2][2][2]) {
    unsigned acc = 0;
    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                const struct Cell *cell = &cells[x][y][z];
                acc = acc * 53u +
                      (unsigned char)cell->payload.text[0] * 13u +
                      (unsigned char)cell->payload.text[1] * 11u +
                      (unsigned char)cell->payload.text[2] * 7u +
                      (unsigned char)cell->payload.text[3] * 5u +
                      (unsigned)cell->mark;
            }
        }
    }
    return acc;
}

int main(void) {
    struct CellCube cube = {
        .cells[0][0][0].payload.text = "ab",
        .cells[0][0][0].mark = 1,
        .cells[0][0][0].payload.chars.d = 'Q',
        .cells[0][0][0].payload.text = "uv",
        .cells[0][0][0].payload.chars.b = 'x',
        .cells[0][1][1].payload.chars = { 'm', 'n', 'o', 'p', 0, 0, 0, 0 },
        .cells[0][1][1].mark = 3,
        .cells[0][1][1].payload.text = "io",
        .cells[0][1][1].payload.chars.c = '!',
        .cells[1][0][0].payload.text = "jk",
        .cells[1][0][0].mark = 5,
        .cells[1][0][0].payload.chars.g = '?',
        .cells[1][0][0].payload.text = "rs",
        .cells[1][0][0].payload.chars.a = 'A',
        .cells[1][1][1].payload.chars = { 'q', 'r', 's', 0, 0, 0, 0, 0 },
        .cells[1][1][1].mark = 7,
        .cells[1][1][1].payload.text = "lm",
        .cells[1][1][1].payload.chars.b = 'p',
    };

    printf("%d %d %d %u\n",
           cube.cells[0][0][0].payload.text[1],
           cube.cells[0][1][1].payload.text[2],
           cube.cells[1][1][1].payload.text[1],
           checksum(cube.cells));
    return 0;
}
