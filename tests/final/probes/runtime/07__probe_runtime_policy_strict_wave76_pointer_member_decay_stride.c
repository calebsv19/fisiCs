#include <stddef.h>
#include <stdio.h>

enum Lane {
    LANE_A = 1,
    LANE_B = 4
};

struct Payload {
    unsigned short lane[2][4];
    unsigned char tag;
};

static int row_fold(unsigned short (*row)[4], enum Lane lane) {
    unsigned short *flat = *row;
    return (int)flat[0] + (int)flat[3] + (int)lane;
}

int main(void) {
    struct Payload payloads[2] = {
        {{{3u, 5u, 7u, 11u}, {13u, 17u, 19u, 23u}}, 4u},
        {{{29u, 31u, 37u, 41u}, {43u, 47u, 53u, 59u}}, 6u}
    };

    unsigned short (*selected)[4] = 1 ? &payloads[1].lane[0] : &payloads[0].lane[0];
    void *opaque = (void *)selected;
    unsigned short (*roundtrip)[4] = (unsigned short (*)[4])opaque;
    unsigned short *flat = *roundtrip;
    ptrdiff_t row_delta = roundtrip - &payloads[1].lane[0];
    ptrdiff_t cell_delta = &flat[3] - &flat[0];
    int adjacent = &roundtrip[0][4] == &roundtrip[1][0];
    unsigned int mixed = (unsigned int)(unsigned short)(flat[2] + payloads[1].tag + payloads[0].lane[1][1]);

    printf("%d %ld %ld %d %u\n",
           row_fold(roundtrip, LANE_B),
           (long)row_delta,
           (long)cell_delta,
           adjacent,
           mixed);
    return 0;
}
