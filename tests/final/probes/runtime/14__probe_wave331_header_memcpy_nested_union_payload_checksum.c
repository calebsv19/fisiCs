#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct Cell {
    uint16_t tag;
    union {
        struct {
            unsigned char a;
            unsigned char b;
            unsigned char c;
            unsigned char d;
        };
        unsigned char raw[4];
    } payload;
} Cell;

typedef struct Page {
    uint32_t epoch;
    Cell cells[3];
    size_t stride;
} Page;

static Page make_page(unsigned char seed) {
    Page out;
    size_t i;

    out.epoch = 0x33440000u + (uint32_t)seed;
    for (i = 0u; i < 3u; ++i) {
        out.cells[i].tag = (uint16_t)(0x2100u + seed + i * 13u);
        out.cells[i].payload.raw[0] = (unsigned char)(0x10u + seed + i);
        out.cells[i].payload.raw[1] = (unsigned char)(0x30u + seed + i * 3u);
        out.cells[i].payload.raw[2] = (unsigned char)(0x50u + seed + i * 5u);
        out.cells[i].payload.raw[3] = (unsigned char)(0x70u + seed + i * 7u);
    }
    out.stride = offsetof(Page, cells) + sizeof(out.cells[1]);
    return out;
}

static unsigned fold_page(Page value) {
    unsigned acc = (unsigned)value.epoch + (unsigned)value.stride * 17u;
    size_t i;
    size_t j;

    for (i = 0u; i < 3u; ++i) {
        acc = acc * 131u + (unsigned)value.cells[i].tag + (unsigned)i;
        for (j = 0u; j < 4u; ++j) {
            acc = acc * 97u + (unsigned)value.cells[i].payload.raw[j] + (unsigned)j;
        }
    }
    return acc;
}

int main(void) {
    Page page = make_page(9u);
    Page copy;
    unsigned typed = 0u;

    memset(&copy, 0, sizeof(copy));
    memcpy(&copy, &page, sizeof(copy));
    copy.cells[1].payload.b ^= (unsigned char)(copy.cells[0].payload.raw[2] + 3u);
    copy.cells[2].payload.raw[3] = (unsigned char)(copy.cells[2].payload.raw[3] + copy.cells[1].payload.a);

    typed += (unsigned)copy.epoch + (unsigned)copy.stride;
    typed += (unsigned)copy.cells[0].tag * 3u + (unsigned)copy.cells[0].payload.a * 5u;
    typed += (unsigned)copy.cells[1].payload.b * 7u + (unsigned)copy.cells[2].payload.raw[3] * 11u;

    printf("%u %u %u %u %u %u %u %u\n",
           (unsigned)copy.epoch,
           (unsigned)copy.cells[0].tag,
           (unsigned)copy.cells[0].payload.raw[2],
           (unsigned)copy.cells[1].payload.raw[1],
           (unsigned)copy.cells[2].payload.raw[3],
           (unsigned)copy.stride,
           typed,
           fold_page(copy));
    return 0;
}
