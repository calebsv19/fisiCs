#include <stdio.h>

struct Cell {
    unsigned x : 4;
    unsigned y : 4;
    unsigned active : 1;
    unsigned spare : 7;
};

static unsigned checksum(const struct Cell *cells, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        acc = acc * 29u + cells[i].x * 7u + cells[i].y * 3u + cells[i].active;
    }
    return acc;
}

int main(void) {
    struct Cell cells[3] = {
        [1] = {
            .y = 7,
            .active = 1,
        },
        [2] = {
            .x = 5,
        },
    };

    printf(
        "%u %u %u\n",
        cells[0].x + cells[0].y + cells[0].active,
        cells[1].x + cells[1].y + cells[1].active,
        checksum(cells, 3)
    );
    return 0;
}
