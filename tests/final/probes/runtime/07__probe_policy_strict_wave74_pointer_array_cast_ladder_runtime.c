#include <stddef.h>
#include <stdio.h>

struct Packet {
    unsigned char lanes[3][4];
};

static int checksum(unsigned char (*row)[4]) {
    void *opaque = (void *)row;
    unsigned char (*roundtrip)[4] = (unsigned char (*)[4])opaque;
    unsigned char *flat = *roundtrip;
    unsigned int total = 0u;
    int i;
    for (i = 0; i < 4; i++) {
        total += (unsigned int)(unsigned char)(flat[i] + (unsigned char)i);
    }
    return (int)total;
}

int main(void) {
    struct Packet packets[2] = {
        {{{1u, 2u, 3u, 4u}, {5u, 6u, 7u, 8u}, {9u, 10u, 11u, 12u}}},
        {{{21u, 22u, 23u, 24u}, {31u, 32u, 33u, 34u}, {41u, 42u, 43u, 44u}}}
    };

    unsigned char (*base)[4] = packets[1].lanes;
    unsigned char (*selected)[4] = 1 ? &base[2] : &packets[0].lanes[0];
    void *opaque = (void *)(selected - 1);
    unsigned char (*roundtrip)[4] = (unsigned char (*)[4])opaque;
    ptrdiff_t row_delta = selected - packets[1].lanes;
    ptrdiff_t cell_delta = &(*selected)[3] - &(*selected)[0];
    int contiguous = (&roundtrip[0][4] == &roundtrip[1][0]);

    printf("%d %ld %ld %d %u\n",
           checksum(roundtrip),
           (long)row_delta,
           (long)cell_delta,
           contiguous,
           (unsigned int)(unsigned char)((*selected)[3] + packets[0].lanes[2][1]));
    return 0;
}
