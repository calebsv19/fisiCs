#include <stdio.h>

typedef union CellValue {
    unsigned char bytes[6];
    unsigned short half[3];
} CellValue;

typedef struct Row {
    CellValue value[2];
    unsigned short bias;
} Row;

typedef struct Grid {
    Row row[3];
    unsigned short stamp;
} Grid;

static Grid seed_grid(unsigned seed) {
    Grid out;
    unsigned r;
    unsigned c;
    unsigned k;

    for (r = 0u; r < 3u; ++r) {
        out.row[r].bias = (unsigned short)(0x1800u + seed + r * 0x41u);
        for (c = 0u; c < 2u; ++c) {
            for (k = 0u; k < 6u; ++k) {
                out.row[r].value[c].bytes[k] = (unsigned char)(0x2du + seed * 3u + r * 17u + c * 7u + k * 5u);
            }
        }
    }
    out.stamp = (unsigned short)(0x5400u + seed * 29u);
    return out;
}

static Grid rewrite_grid(Grid in, unsigned salt) {
    Grid out = in;
    Row hold = out.row[0];

    out.row[0] = out.row[2];
    out.row[2] = out.row[1];
    out.row[1] = hold;
    out.row[0].value[1].half[2] = (unsigned short)(out.row[0].value[1].half[2] + salt);
    out.row[1].value[0].bytes[4] = (unsigned char)(out.row[1].value[0].bytes[4] ^ (unsigned char)(salt >> 2));
    out.row[2].bias = (unsigned short)(out.row[2].bias + out.row[0].value[1].bytes[5]);
    out.stamp = (unsigned short)(out.stamp + out.row[1].value[0].bytes[4] + out.row[2].bias);
    return out;
}

static unsigned checksum(Grid value) {
    unsigned acc = (unsigned)value.stamp * 167u;
    unsigned r;
    unsigned c;
    unsigned k;

    for (r = 0u; r < 3u; ++r) {
        acc = acc * 103u + (unsigned)value.row[r].bias;
        for (c = 0u; c < 2u; ++c) {
            for (k = 0u; k < 6u; ++k) {
                acc = acc * 103u + (unsigned)value.row[r].value[c].bytes[k] + c + k;
            }
        }
    }
    return acc;
}

int main(void) {
    Grid grid = seed_grid(11u);
    Grid copy = rewrite_grid(rewrite_grid(grid, 0x44u), 0x2eu);
    unsigned typed = 0u;

    typed += (unsigned)copy.row[0].bias * 3u;
    typed += (unsigned)copy.row[0].value[1].half[2] * 5u;
    typed += (unsigned)copy.row[1].value[0].bytes[4] * 7u;
    typed += (unsigned)copy.row[2].bias * 11u;
    typed += (unsigned)copy.stamp;

    printf("%u %u %u %u %u %u %u\n",
           (unsigned)copy.row[0].bias,
           (unsigned)copy.row[0].value[1].half[2],
           (unsigned)copy.row[1].value[0].bytes[4],
           (unsigned)copy.row[2].bias,
           (unsigned)copy.stamp,
           typed,
           checksum(copy));
    return 0;
}
