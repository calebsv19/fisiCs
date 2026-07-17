#include <stdio.h>

typedef struct Cell {
    unsigned char id;
    union {
        struct {
            unsigned char lo;
            unsigned char hi;
        };
        unsigned short word;
        unsigned char raw[2];
    } payload;
} Cell;

typedef struct Frame {
    unsigned char epoch;
    Cell cells[4];
    unsigned char tail;
} Frame;

typedef Frame (*FrameOp)(Frame, unsigned char);

static unsigned fold_cell(Cell value, unsigned salt) {
    unsigned acc = salt + (unsigned)value.id * 29u;
    acc = acc * 131u + (unsigned)value.payload.raw[0];
    acc = acc * 131u + (unsigned)value.payload.raw[1];
    return acc;
}

static unsigned fold_frame(Frame value) {
    unsigned acc = (unsigned)value.epoch + 17u;
    unsigned i;

    for (i = 0u; i < 4u; ++i) {
        acc ^= fold_cell(value.cells[i], i + 3u);
        acc = acc * 167u + (unsigned)(i + 11u);
    }
    return acc ^ ((unsigned)value.tail << 8);
}

static Frame seed_frame(unsigned char seed) {
    Frame out;
    unsigned i;

    out.epoch = (unsigned char)(0x30u + seed);
    for (i = 0u; i < 4u; ++i) {
        out.cells[i].id = (unsigned char)(seed + i * 7u);
        out.cells[i].payload.raw[0] = (unsigned char)(0x20u + seed + i * 3u);
        out.cells[i].payload.raw[1] = (unsigned char)(0x70u + seed + i * 5u);
    }
    out.tail = (unsigned char)(0xA0u + seed);
    return out;
}

static Frame rotate_payloads(Frame in, unsigned char salt) {
    Frame out = in;
    Cell saved = out.cells[0];

    out.cells[0] = out.cells[2];
    out.cells[2] = out.cells[3];
    out.cells[3] = saved;
    out.cells[1].payload.raw[0] ^= salt;
    out.cells[1].payload.raw[1] = (unsigned char)(out.cells[1].payload.raw[1] + salt);
    return out;
}

static Frame word_rewrite(Frame in, unsigned char salt) {
    Frame out = in;

    out.cells[0].payload.word = (unsigned short)(out.cells[0].payload.word + (unsigned short)(salt * 13u));
    out.cells[2].payload.raw[0] ^= (unsigned char)(salt + out.cells[3].id);
    out.tail = (unsigned char)(out.tail + out.cells[0].payload.raw[0] + out.cells[2].payload.raw[1]);
    return out;
}

static Frame table_run(Frame input) {
    FrameOp ops[3];
    Frame value = input;
    unsigned i;

    ops[0] = rotate_payloads;
    ops[1] = word_rewrite;
    ops[2] = rotate_payloads;
    for (i = 0u; i < 3u; ++i) {
        value = ops[i](value, (unsigned char)(0x11u + i * 9u));
    }
    return value;
}

int main(void) {
    Frame result = table_run(seed_frame(5u));
    unsigned typed = 0u;
    unsigned i;

    for (i = 0u; i < 4u; ++i) {
        typed += (unsigned)result.cells[i].id * (i + 5u);
        typed += (unsigned)result.cells[i].payload.lo * (i + 17u);
        typed += (unsigned)result.cells[i].payload.hi * (i + 23u);
    }

    printf("%u %u %u %u %u %u %u %u %u %u\n",
           (unsigned)result.epoch,
           (unsigned)result.cells[0].id,
           (unsigned)result.cells[0].payload.raw[0],
           (unsigned)result.cells[0].payload.raw[1],
           (unsigned)result.cells[1].id,
           (unsigned)result.cells[1].payload.raw[0],
           (unsigned)result.cells[2].payload.raw[0],
           (unsigned)result.cells[3].payload.raw[1],
           typed,
           fold_frame(result));
    return 0;
}
