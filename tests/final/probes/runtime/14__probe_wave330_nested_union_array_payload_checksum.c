#include <stdio.h>

typedef struct Packet {
    unsigned char tag;
    union {
        struct {
            unsigned char lo;
            unsigned char hi;
            unsigned char aux;
        };
        unsigned char bytes[3];
    } payload;
} Packet;

typedef struct Page {
    unsigned short epoch;
    Packet rows[2][3];
    unsigned short tail;
} Page;

static unsigned fold_page(Page value) {
    unsigned acc = (unsigned)value.epoch * 211u + (unsigned)value.tail;
    unsigned i;
    unsigned j;
    unsigned k;

    for (i = 0u; i < 2u; ++i) {
        for (j = 0u; j < 3u; ++j) {
            acc = acc * 173u + (unsigned)value.rows[i][j].tag + i * 7u + j;
            for (k = 0u; k < 3u; ++k) {
                acc = acc * 101u + (unsigned)value.rows[i][j].payload.bytes[k] + k;
            }
        }
    }
    return acc;
}

static Page seed_page(unsigned char seed) {
    Page out;
    unsigned i;
    unsigned j;

    out.epoch = (unsigned short)(0x2100u + seed);
    for (i = 0u; i < 2u; ++i) {
        for (j = 0u; j < 3u; ++j) {
            out.rows[i][j].tag = (unsigned char)(seed + 0x20u + i * 9u + j * 5u);
            out.rows[i][j].payload.bytes[0] = (unsigned char)(0x31u + seed + i + j);
            out.rows[i][j].payload.bytes[1] = (unsigned char)(0x62u + seed + i * 3u + j * 7u);
            out.rows[i][j].payload.bytes[2] = (unsigned char)(0x93u + seed + i * 5u + j * 11u);
        }
    }
    out.tail = (unsigned short)(0x4300u + seed * 3u);
    return out;
}

static Page rotate_page(Page in, unsigned char salt) {
    Page out = in;
    Packet saved = out.rows[0][0];

    out.rows[0][0] = out.rows[1][2];
    out.rows[1][2] = out.rows[1][0];
    out.rows[1][0] = saved;
    out.rows[0][1].payload.lo ^= salt;
    out.rows[1][1].payload.hi = (unsigned char)(out.rows[1][1].payload.hi + salt);
    out.tail = (unsigned short)(out.tail + out.rows[0][1].payload.aux + out.rows[1][1].payload.lo);
    return out;
}

int main(void) {
    Page page = rotate_page(seed_page(6u), 0x2Du);
    unsigned typed = 0u;

    typed += (unsigned)page.epoch + (unsigned)page.tail;
    typed += (unsigned)page.rows[0][0].tag * 3u + (unsigned)page.rows[0][0].payload.lo * 5u;
    typed += (unsigned)page.rows[0][1].payload.bytes[0] * 7u + (unsigned)page.rows[1][1].payload.hi * 11u;
    typed += (unsigned)page.rows[1][2].tag * 13u + (unsigned)page.rows[1][2].payload.aux * 17u;

    printf("%u %u %u %u %u %u %u %u %u %u\n",
           (unsigned)page.epoch,
           (unsigned)page.rows[0][0].tag,
           (unsigned)page.rows[0][0].payload.bytes[0],
           (unsigned)page.rows[0][1].payload.bytes[0],
           (unsigned)page.rows[1][0].tag,
           (unsigned)page.rows[1][1].payload.bytes[1],
           (unsigned)page.rows[1][2].tag,
           (unsigned)page.tail,
           typed,
           fold_page(page));
    return 0;
}
