#include <stdio.h>

typedef struct Lane {
    unsigned char key;
    union {
        struct {
            unsigned char lo;
            unsigned char hi;
            unsigned char aux;
        };
        unsigned char raw[3];
    } payload;
} Lane;

typedef struct Frame {
    unsigned short seq;
    Lane lanes[2][2];
    unsigned short guard;
} Frame;

typedef Frame (*FrameOp)(Frame, unsigned char);

static Frame seed_frame(unsigned char seed) {
    Frame out;
    unsigned i;
    unsigned j;

    out.seq = (unsigned short)(0x1800u + seed);
    for (i = 0u; i < 2u; ++i) {
        for (j = 0u; j < 2u; ++j) {
            out.lanes[i][j].key = (unsigned char)(0x20u + seed + i * 11u + j * 5u);
            out.lanes[i][j].payload.raw[0] = (unsigned char)(0x41u + seed + i + j);
            out.lanes[i][j].payload.raw[1] = (unsigned char)(0x61u + seed + i * 3u + j * 7u);
            out.lanes[i][j].payload.raw[2] = (unsigned char)(0x81u + seed + i * 5u + j * 9u);
        }
    }
    out.guard = (unsigned short)(0x3900u + seed * 5u);
    return out;
}

static Frame mix_a(Frame in, unsigned char salt) {
    Frame out = in;
    Lane saved = out.lanes[0][0];

    out.lanes[0][0] = out.lanes[1][1];
    out.lanes[1][1] = saved;
    out.lanes[0][1].payload.lo = (unsigned char)(out.lanes[0][1].payload.lo + salt);
    out.guard = (unsigned short)(out.guard + out.lanes[1][1].payload.aux);
    return out;
}

static Frame mix_b(Frame in, unsigned char salt) {
    Frame out = in;

    out.lanes[1][0].payload.hi ^= (unsigned char)(salt + out.lanes[0][0].key);
    out.lanes[1][1].payload.raw[2] = (unsigned char)(out.lanes[1][1].payload.raw[2] + out.lanes[0][1].payload.lo);
    out.seq = (unsigned short)(out.seq + out.lanes[1][0].payload.raw[0]);
    return out;
}

static FrameOp choose(unsigned index) {
    FrameOp table[2];

    table[0] = mix_a;
    table[1] = mix_b;
    return table[index & 1u];
}

static unsigned fold_frame(Frame value) {
    unsigned acc = (unsigned)value.seq * 193u + (unsigned)value.guard;
    unsigned i;
    unsigned j;
    unsigned k;

    for (i = 0u; i < 2u; ++i) {
        for (j = 0u; j < 2u; ++j) {
            acc = acc * 109u + (unsigned)value.lanes[i][j].key;
            for (k = 0u; k < 3u; ++k) {
                acc = acc * 83u + (unsigned)value.lanes[i][j].payload.raw[k];
            }
        }
    }
    return acc;
}

int main(void) {
    Frame frame = seed_frame(7u);
    unsigned i;
    unsigned typed = 0u;

    for (i = 0u; i < 5u; ++i) {
        FrameOp op = choose(i + 1u);
        frame = op(frame, (unsigned char)(0x12u + i * 3u));
    }

    typed += (unsigned)frame.seq + (unsigned)frame.guard;
    typed += (unsigned)frame.lanes[0][0].key * 3u + (unsigned)frame.lanes[0][0].payload.lo * 5u;
    typed += (unsigned)frame.lanes[0][1].payload.hi * 7u + (unsigned)frame.lanes[1][0].payload.raw[1] * 11u;
    typed += (unsigned)frame.lanes[1][1].payload.aux * 13u;

    printf("%u %u %u %u %u %u %u %u %u\n",
           (unsigned)frame.seq,
           (unsigned)frame.lanes[0][0].key,
           (unsigned)frame.lanes[0][0].payload.raw[0],
           (unsigned)frame.lanes[0][1].payload.raw[1],
           (unsigned)frame.lanes[1][0].payload.raw[1],
           (unsigned)frame.lanes[1][1].payload.raw[2],
           (unsigned)frame.guard,
           typed,
           fold_frame(frame));
    return 0;
}
