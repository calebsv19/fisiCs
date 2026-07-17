#include <stdio.h>

typedef struct Packet { unsigned int x, y, z; } Packet;

static Packet make_packet(unsigned int seed) {
    Packet p = {seed * 3u + 1u, seed * 5u + 2u, seed * 7u + 3u};
    return p;
}

static unsigned int checksum(Packet p) {
    return p.x * 17u + p.y * 31u + p.z * 43u;
}

int main(void) {
    Packet direct = make_packet(19u);
    Packet staged;
    Packet temporary = make_packet(19u);
    staged = temporary;
    printf("%u %u %u\n", checksum(direct), checksum(staged), checksum(direct) == checksum(staged));
    return 0;
}
