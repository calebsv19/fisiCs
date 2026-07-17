#include <stdio.h>

#include "13__probe_wave59_include_line_parser_recovery_runtime.h"

struct Wave59Packet {
    int lane[3];
    union {
        int scalar;
        int pair[2];
    } payload;
};

static struct Wave59Packet wave59_build_packet(int seed) {
    struct Wave59Packet packet = {{seed, seed + 5, seed * 3}, {0}};
    packet.payload.pair[0] = packet.lane[0] + packet.lane[1];
    packet.payload.pair[1] = packet.lane[2] - packet.lane[0];
    return packet;
}

static int wave59_packet_checksum(struct Wave59Packet packet) {
    struct Wave59Packet copy = packet;
    copy.lane[1] += copy.payload.pair[1];
    return copy.lane[0] + copy.lane[1] + copy.lane[2] +
           copy.payload.pair[0] + copy.payload.pair[1];
}

int main(void) {
    int seed = wave59_header_seed(0);
    struct Wave59Packet packet = wave59_build_packet(seed);
    int checksum = wave59_packet_checksum(packet);

    printf("%d %d %d %d %d\n",
           seed,
           packet.lane[1],
           packet.lane[2],
           packet.payload.pair[0],
           checksum);
    return checksum == 54 ? 0 : 1;
}
