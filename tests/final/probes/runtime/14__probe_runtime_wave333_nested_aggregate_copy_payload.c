#include <stdio.h>

typedef union PayloadView {
    unsigned char bytes[6];
    unsigned short half[3];
} PayloadView;

typedef struct Cell {
    unsigned char tag;
    PayloadView view;
} Cell;

typedef struct Block {
    Cell cells[3];
    unsigned short checksum;
} Block;

static Cell make_cell(unsigned seed, unsigned tag) {
    Cell out;
    unsigned i;

    out.tag = (unsigned char)(tag + seed);
    for (i = 0u; i < 6u; ++i) {
        out.view.bytes[i] = (unsigned char)(0x17u + seed * 11u + i * 7u);
    }
    return out;
}

static Block make_block(unsigned seed) {
    Block out;
    unsigned i;

    for (i = 0u; i < 3u; ++i) {
        out.cells[i] = make_cell(seed + i * 3u, 0x31u + i * 5u);
    }
    out.checksum = (unsigned short)(0x6200u + seed * 19u);
    return out;
}

static Block rewrite(Block in, unsigned salt) {
    Block out = in;
    Cell tmp = out.cells[0];

    out.cells[0] = out.cells[2];
    out.cells[2] = tmp;
    out.cells[1].view.bytes[2] = (unsigned char)(out.cells[1].view.bytes[2] ^ salt);
    out.cells[2].view.half[1] = (unsigned short)(out.cells[2].view.half[1] + out.cells[0].tag);
    out.checksum = (unsigned short)(out.checksum + out.cells[1].view.bytes[2] + out.cells[2].view.bytes[3]);
    return out;
}

static unsigned fold(Block value) {
    unsigned acc = (unsigned)value.checksum * 149u;
    unsigned i;
    unsigned k;

    for (i = 0u; i < 3u; ++i) {
        acc = acc * 131u + (unsigned)value.cells[i].tag;
        for (k = 0u; k < 6u; ++k) {
            acc = acc * 97u + (unsigned)value.cells[i].view.bytes[k] + k;
        }
    }
    return acc;
}

int main(void) {
    Block block = make_block(5u);
    Block copy = rewrite(rewrite(block, 0x2Du), 0x13u);
    unsigned typed = 0u;

    typed += (unsigned)copy.cells[0].tag * 3u;
    typed += (unsigned)copy.cells[1].view.bytes[2] * 5u;
    typed += (unsigned)copy.cells[2].view.half[1] * 7u;
    typed += (unsigned)copy.checksum;

    printf("%u %u %u %u %u %u %u\n",
           (unsigned)copy.cells[0].tag,
           (unsigned)copy.cells[0].view.bytes[0],
           (unsigned)copy.cells[1].view.bytes[2],
           (unsigned)copy.cells[2].view.bytes[3],
           (unsigned)copy.checksum,
           typed,
           fold(copy));
    return 0;
}
