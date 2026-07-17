#include <stdio.h>

typedef struct Cell {
    unsigned char key;
    union {
        struct {
            unsigned char p;
            unsigned char q;
            unsigned char r;
            unsigned char s;
        };
        unsigned char bytes[4];
    } u;
} Cell;

typedef struct Frame {
    unsigned short mark;
    Cell cells[3];
    unsigned short trailer;
} Frame;

typedef Cell (*CellOp)(Cell, unsigned char);

static Cell make_cell(unsigned char seed) {
    Cell out;

    out.key = (unsigned char)(0x30u + seed);
    out.u.bytes[0] = (unsigned char)(seed + 0x11u);
    out.u.bytes[1] = (unsigned char)(seed + 0x22u);
    out.u.bytes[2] = (unsigned char)(seed + 0x33u);
    out.u.bytes[3] = (unsigned char)(seed + 0x44u);
    return out;
}

static Cell twist(Cell in, unsigned char salt) {
    Cell out = in;

    out.u.p ^= salt;
    out.u.r = (unsigned char)(out.u.r + out.key);
    return out;
}

static Cell lift(Cell in, unsigned char salt) {
    Cell out = in;

    out.u.q = (unsigned char)(out.u.q + salt);
    out.u.s ^= (unsigned char)(out.u.p + salt);
    out.key = (unsigned char)(out.key + out.u.s);
    return out;
}

static Frame run_table(Frame input, const unsigned char plan[4]) {
    CellOp ops[2];
    Frame out = input;
    unsigned i;

    ops[0] = twist;
    ops[1] = lift;
    for (i = 0u; i < 4u; ++i) {
        Cell next = ops[plan[i] & 1u](out.cells[(i + 1u) % 3u], (unsigned char)(0x17u + i * 11u));
        out.cells[i % 3u] = next;
        out.trailer = (unsigned short)(out.trailer + next.u.bytes[i & 3u] + next.key);
    }
    return out;
}

static unsigned fold_frame(Frame value) {
    unsigned acc = (unsigned)value.mark * 199u + (unsigned)value.trailer;
    unsigned i;
    unsigned j;

    for (i = 0u; i < 3u; ++i) {
        acc = acc * 151u + (unsigned)value.cells[i].key;
        for (j = 0u; j < 4u; ++j) {
            acc = acc * 83u + (unsigned)value.cells[i].u.bytes[j] + i * 5u + j;
        }
    }
    return acc;
}

int main(void) {
    unsigned char plan[4] = { 1u, 0u, 1u, 0u };
    Frame frame;
    unsigned typed = 0u;

    frame.mark = 0x2A00u;
    frame.cells[0] = make_cell(2u);
    frame.cells[1] = make_cell(9u);
    frame.cells[2] = make_cell(14u);
    frame.trailer = 0x5C00u;

    frame = run_table(frame, plan);

    typed += (unsigned)frame.mark + (unsigned)frame.trailer;
    typed += (unsigned)frame.cells[0].key * 3u + (unsigned)frame.cells[0].u.p * 5u;
    typed += (unsigned)frame.cells[1].u.q * 7u + (unsigned)frame.cells[1].u.s * 11u;
    typed += (unsigned)frame.cells[2].key * 13u + (unsigned)frame.cells[2].u.bytes[2] * 17u;

    printf("%u %u %u %u %u %u %u %u %u %u\n",
           (unsigned)frame.mark,
           (unsigned)frame.cells[0].key,
           (unsigned)frame.cells[0].u.bytes[0],
           (unsigned)frame.cells[0].u.bytes[3],
           (unsigned)frame.cells[1].key,
           (unsigned)frame.cells[1].u.bytes[1],
           (unsigned)frame.cells[2].u.bytes[2],
           (unsigned)frame.trailer,
           typed,
           fold_frame(frame));
    return 0;
}
