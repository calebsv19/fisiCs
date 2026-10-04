#include <stdio.h>

struct CountPacket {
    long double value;
    unsigned tag;
};

struct CountPacket collect_counted(long double seed, unsigned tag, int count, ...);

static void show(int count, struct CountPacket packet) {
    unsigned scaled = (unsigned)(packet.value * 256.0L + 0.5L);
    printf("%d %u %u\n", count, scaled, packet.tag);
}

int main(void) {
    show(0, collect_counted(1.0L, 17u, 0));
    show(1, collect_counted(1.0L, 17u, 1,
                            0.5L));
    show(8, collect_counted(1.0L, 17u, 8,
                            0.5L, 3, 1.25L, 5, 2.0L, 7, 2.75L, 11));
    show(9, collect_counted(1.0L, 17u, 9,
                            0.5L, 3, 1.25L, 5, 2.0L, 7, 2.75L, 11, 3.5L));
    show(10, collect_counted(1.0L, 17u, 10,
                             0.5L, 3, 1.25L, 5, 2.0L, 7, 2.75L, 11, 3.5L, 13));
    return 0;
}
