#include <stdio.h>

typedef struct Item {
    unsigned char key;
    union {
        struct {
            unsigned char x;
            unsigned char y;
            unsigned char z;
        };
        unsigned char raw[3];
    } payload;
} Item;

typedef struct Grid {
    unsigned short mark;
    Item row[3];
} Grid;

static Grid transform(Grid in, unsigned char salt) {
    Grid out = in;

    out.row[0] = (Item){ (unsigned char)(out.row[2].key + salt),
                         { { out.row[1].payload.z, out.row[0].payload.y, out.row[2].payload.x } } };
    out.row[1].payload.raw[0] = (unsigned char)(out.row[1].payload.raw[0] + out.row[0].payload.raw[2]);
    out.row[2] = (Item){ (unsigned char)(out.row[1].key ^ salt),
                         { { out.row[0].payload.raw[1], out.row[1].payload.raw[2], out.row[2].payload.raw[0] } } };
    out.mark = (unsigned short)(out.mark + out.row[0].key + out.row[2].payload.z);
    return out;
}

static unsigned fold_grid(Grid value) {
    unsigned acc = (unsigned)value.mark * 167u;
    unsigned i;
    unsigned k;

    for (i = 0u; i < 3u; ++i) {
        acc = acc * 131u + (unsigned)value.row[i].key;
        for (k = 0u; k < 3u; ++k) {
            acc = acc * 97u + (unsigned)value.row[i].payload.raw[k] + k;
        }
    }
    return acc;
}

int main(void) {
    Grid grid = {
        0x4406u,
        {
            { 0x12u, { { 0x21u, 0x31u, 0x41u } } },
            { 0x23u, { { 0x52u, 0x62u, 0x72u } } },
            { 0x34u, { { 0x83u, 0x93u, 0xa3u } } }
        }
    };
    unsigned typed = 0u;

    grid = transform(grid, 0x19u);
    grid.row[1] = transform(grid, 0x07u).row[0];
    grid = transform(grid, 0x2bu);

    typed += (unsigned)grid.mark;
    typed += (unsigned)grid.row[0].key * 3u + (unsigned)grid.row[0].payload.raw[0] * 5u;
    typed += (unsigned)grid.row[1].payload.y * 7u + (unsigned)grid.row[2].payload.z * 11u;

    printf("%u %u %u %u %u %u %u %u\n",
           (unsigned)grid.mark,
           (unsigned)grid.row[0].key,
           (unsigned)grid.row[0].payload.raw[0],
           (unsigned)grid.row[1].key,
           (unsigned)grid.row[1].payload.raw[1],
           (unsigned)grid.row[2].payload.raw[2],
           typed,
           fold_grid(grid));
    return 0;
}
