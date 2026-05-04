#include <stdio.h>

union Payload {
    char text[8];
    struct {
        unsigned char head;
        unsigned char mid;
        unsigned char tail;
    } bytes;
};

struct Cell {
    union Payload payload;
    int mark;
};

struct Grid {
    struct Cell cells[2][2][2];
};

static int checksum(const struct Grid *grids, int count) {
    int acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const struct Cell *cell = &grids[i].cells[x][y][z];
                    acc = acc * 31 +
                          cell->payload.text[0] * 7 +
                          cell->payload.text[1] * 5 +
                          cell->payload.text[2] * 3 +
                          cell->mark;
                }
            }
        }
    }

    return acc;
}

int main(void) {
    struct Grid grids[2] = {
        [0] = {
            .cells[0][1][0].payload.text = "alpha",
            .cells[0][1][0].payload.bytes.mid = 'Z',
            .cells[1][0][1].payload.bytes = {
                .head = 'q',
                .mid = 'r',
                .tail = 's',
            },
            .cells[1][0][1].payload.text = "uv",
            .cells[1][0][1].payload.bytes.tail = '!',
            .cells[1][0][1].mark = 4,
        },
        [1] = {
            .cells[1][1][1].payload.bytes = {
                .head = 'm',
                .mid = 'n',
                .tail = 'o',
            },
            .cells[1][1][1].payload.text = "xy",
            .cells[1][1][1].payload.bytes.head = 'K',
            .cells[0][0][1].payload.text = "cat",
            .cells[0][0][1].payload.bytes.tail = 'T',
            .cells[0][0][1].mark = 6,
        },
    };

    printf("%d %d %d %d\n",
           grids[0].cells[0][1][0].payload.bytes.mid,
           grids[0].cells[1][0][1].payload.bytes.tail,
           grids[1].cells[1][1][1].payload.bytes.head,
           checksum(grids, 2));
    return 0;
}
