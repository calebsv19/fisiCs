#include <stdio.h>

typedef union {
    struct {
        int a;
        int b;
    } pair;
    int raw[2];
} Wave45Payload;

typedef struct {
    int tag;
    Wave45Payload payload;
    int trail[3];
} Wave45Packet;

static Wave45Packet pair_packet(int seed) {
    Wave45Packet packet = {1, {.pair = {seed + 2, seed * 3 - 1}}, {seed, seed + 5, seed + 11}};
    return packet;
}

static Wave45Packet raw_packet(int seed) {
    Wave45Packet packet = {2, {.raw = {seed * 4 + 3, seed - 7}}, {seed + 13, seed + 17, seed + 19}};
    return packet;
}

static Wave45Packet choose_packet(Wave45Packet current, int step) {
    switch ((current.trail[step % 3] + step + current.tag) % 5) {
        case 0: {
            Wave45Packet next = pair_packet(step + current.trail[0] % 9);
            next.trail[1] += current.payload.raw[0];
            return next;
        }
        case 1: {
            Wave45Packet next = raw_packet(step + current.trail[1] % 7);
            next.payload.raw[1] -= current.tag;
            return next;
        }
        case 2:
            current.trail[2] += step + current.payload.raw[1];
            return current;
        default:
            return (step & 1) ? raw_packet(current.tag + step + 3) : pair_packet(current.tag + step + 4);
    }
}

static int checksum(Wave45Packet packet) {
    int total = packet.tag + packet.trail[0] * 3 - packet.trail[1] + packet.trail[2] * 2;
    if (packet.tag == 1) {
        total += packet.payload.pair.a * 5 - packet.payload.pair.b * 2;
    } else {
        total += packet.payload.raw[0] - packet.payload.raw[1] * 4;
    }
    return total;
}

int main(void) {
    Wave45Packet packet = pair_packet(5);
    int total = 0;
    int i;

    for (i = 0; i < 12; ++i) {
        Wave45Packet candidate = choose_packet(packet, i);
        Wave45Packet fallback = packet;
        fallback.trail[i % 3] += checksum(candidate) % 13;

        if ((checksum(candidate) > checksum(packet) - i) || ((i & 3) == 3)) {
            packet = candidate;
        } else {
            packet = fallback;
        }

        total += checksum(packet);
    }

    printf("%d %d %d %d %d\n", packet.tag, packet.trail[0], packet.trail[1], checksum(packet), total);
    return 0;
}
