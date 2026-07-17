#include <stdio.h>

typedef union Overlay {
    unsigned char byte[8];
    unsigned short half[4];
} Overlay;

typedef struct Row {
    unsigned char lane;
    Overlay overlay;
} Row;

typedef struct Grid {
    Row rows[3];
    unsigned short salt;
} Grid;

static Grid seed_grid(unsigned seed) {
    Grid out;
    unsigned r;
    unsigned c;

    for (r = 0u; r < 3u; ++r) {
        out.rows[r].lane = (unsigned char)(seed + r * 9u);
        for (c = 0u; c < 8u; ++c) {
            out.rows[r].overlay.byte[c] = (unsigned char)(0x23u + seed * 5u + r * 17u + c * 3u);
        }
    }
    out.salt = (unsigned short)(0x5400u + seed * 31u);
    return out;
}

static Grid overlay_round(Grid in, unsigned round) {
    Grid out = in;
    Row tmp = out.rows[round % 3u];

    out.rows[round % 3u] = out.rows[(round + 1u) % 3u];
    out.rows[(round + 1u) % 3u] = tmp;
    out.rows[2].overlay.half[round & 3u] = (unsigned short)(out.rows[2].overlay.half[round & 3u] ^ (unsigned short)(0x210u + round * 17u));
    out.rows[0].overlay.byte[(round + 5u) & 7u] = (unsigned char)(out.rows[0].overlay.byte[(round + 5u) & 7u] + out.rows[1].lane);
    out.salt = (unsigned short)(out.salt + out.rows[2].overlay.byte[round & 7u] + out.rows[0].overlay.byte[(round + 5u) & 7u]);
    return out;
}

static unsigned fold_grid(Grid value) {
    unsigned acc = (unsigned)value.salt * 191u;
    unsigned r;
    unsigned c;

    for (r = 0u; r < 3u; ++r) {
        acc = acc * 127u + (unsigned)value.rows[r].lane;
        for (c = 0u; c < 8u; ++c) {
            acc = acc * 127u + (unsigned)value.rows[r].overlay.byte[c];
        }
    }
    return acc;
}

int main(void) {
    Grid grid = seed_grid(4u);
    unsigned i;
    unsigned typed = 0u;

    for (i = 0u; i < 4u; ++i) {
        grid = overlay_round(grid, i);
    }

    typed += (unsigned)grid.salt;
    typed += (unsigned)grid.rows[0].lane * 3u + (unsigned)grid.rows[0].overlay.byte[5] * 5u;
    typed += (unsigned)grid.rows[1].overlay.half[2] * 7u + (unsigned)grid.rows[2].overlay.byte[1] * 11u;

    printf("%u %u %u %u %u %u %u %u\n",
           (unsigned)grid.rows[0].lane,
           (unsigned)grid.rows[0].overlay.byte[5],
           (unsigned)grid.rows[1].overlay.byte[4],
           (unsigned)grid.rows[1].overlay.half[2],
           (unsigned)grid.rows[2].overlay.byte[1],
           (unsigned)grid.salt,
           typed,
           fold_grid(grid));
    return 0;
}
