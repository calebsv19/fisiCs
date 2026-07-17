#include <stdio.h>

typedef struct {
    int key;
    union {
        int words[3];
        struct {
            int x;
            int y;
            int z;
        } triple;
    } payload;
    int footer;
} Wave49Packet;

static Wave49Packet packet_from(int seed, int mode) {
    Wave49Packet packet;
    packet.key = seed * 2 + mode;
    packet.footer = seed * 9 - mode;
    if (mode & 1) {
        packet.payload.triple.x = seed + mode * 3;
        packet.payload.triple.y = seed * 2 - mode;
        packet.payload.triple.z = seed ^ (mode + 5);
    } else {
        packet.payload.words[0] = seed + mode;
        packet.payload.words[1] = seed * 3 - mode;
        packet.payload.words[2] = seed * 5 + mode;
    }
    return packet;
}

static Wave49Packet transform(Wave49Packet packet, int step, int mode) {
    if (mode & 1) {
        packet.payload.triple.x += step + packet.footer;
        packet.payload.triple.y -= packet.key - step;
        packet.payload.triple.z += packet.payload.triple.x - packet.payload.triple.y;
        packet.footer += step;
        return packet;
    }
    packet.payload.words[step % 3] += packet.key - packet.footer + step;
    packet.key -= step;
    return (Wave49Packet){packet.key + step, {{packet.payload.words[2], packet.payload.words[0], packet.payload.words[1]}}, packet.footer - step};
}

static int packet_score(Wave49Packet packet, int mode) {
    int total = packet.key * 7 + packet.footer * 11;
    if (mode & 1) {
        total += packet.payload.triple.x * 13 - packet.payload.triple.y * 17 + packet.payload.triple.z * 19;
    } else {
        total += packet.payload.words[0] * 23 - packet.payload.words[1] * 29 + packet.payload.words[2] * 31;
    }
    return total;
}

int main(void) {
    Wave49Packet current = packet_from(3, 0);
    int mode = 0;
    int total = 0;
    int i;

    for (i = 0; i < 12; ++i) {
        Wave49Packet returned = transform(packet_from(i + 4, mode), i, mode);
        Wave49Packet copied = ((packet_score(current, mode) + i) & 1)
            ? transform(current, i + 1, mode)
            : returned;
        mode = (mode + copied.key + i) & 1;
        current = ((packet_score(copied, mode) ^ total) & 1)
            ? copied
            : transform(returned, i + 2, mode);
        total += packet_score(current, mode);
    }

    printf("%d %d %d %d\n", current.key, current.footer, packet_score(current, mode), total);
    return 0;
}
