#include <stdio.h>

typedef union {
    struct {
        int left;
        int right;
    } pair;
    int lanes[2];
} Wave44LoopPayload;

typedef struct {
    int mode;
    Wave44LoopPayload payload;
    int stamp[2];
} Wave44LoopPacket;

static Wave44LoopPacket make_pair_packet(int seed) {
    Wave44LoopPacket packet = {1, {.pair = {seed + 4, seed * 2 - 3}}, {seed + 20, seed + 40}};
    return packet;
}

static Wave44LoopPacket make_lane_packet(int seed) {
    Wave44LoopPacket packet = {2, {.lanes = {seed * 3 + 1, seed - 6}}, {seed + 60, seed + 80}};
    return packet;
}

static Wave44LoopPacket choose_packet(Wave44LoopPacket old, int step) {
    Wave44LoopPacket a = make_pair_packet(old.mode + step + old.stamp[0] % 5);
    Wave44LoopPacket b = make_lane_packet(old.mode + step + old.stamp[1] % 7);
    if (((old.stamp[0] + step) & 3) == 0) {
        a.stamp[1] += old.payload.lanes[0];
        return a;
    }
    b.stamp[0] -= old.mode + step;
    return b;
}

static int score(Wave44LoopPacket packet) {
    if (packet.mode == 1) {
        return packet.payload.pair.left * 7 - packet.payload.pair.right +
               packet.stamp[0] - packet.stamp[1];
    }
    return packet.payload.lanes[0] * 2 + packet.payload.lanes[1] * 5 +
           packet.stamp[1] - packet.stamp[0];
}

int main(void) {
    Wave44LoopPacket packet = make_pair_packet(6);
    int total = 0;
    int i;

    for (i = 0; i < 10; ++i) {
        Wave44LoopPacket candidate = choose_packet(packet, i);
        Wave44LoopPacket copy = packet;

        copy.stamp[i & 1] += score(candidate) % 11;
        if ((score(candidate) > score(copy)) || ((i % 3) == 2)) {
            packet = candidate;
        } else {
            packet = copy;
        }

        if ((i & 3) == 1) {
            Wave44LoopPacket returned = choose_packet(packet, i + 3);
            packet = (score(returned) > score(packet) - i) ? returned : packet;
        }

        total += score(packet);
    }

    printf("%d %d %d %d %d\n", packet.mode, packet.stamp[0], packet.stamp[1], score(packet), total);
    return 0;
}
