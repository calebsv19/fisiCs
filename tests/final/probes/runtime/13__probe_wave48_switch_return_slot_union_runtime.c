#include <stdio.h>

typedef struct {
    int tag;
    union {
        int lane[2];
        struct {
            int left;
            int right;
        } edge;
    } payload;
    int tail;
} Wave48Packet;

static Wave48Packet make_packet(int seed, int mode) {
    Wave48Packet packet;
    packet.tag = seed + mode;
    packet.tail = seed * 5 - mode;
    if (mode & 1) {
        packet.payload.edge.left = seed * 3 + mode;
        packet.payload.edge.right = seed - mode * 2;
    } else {
        packet.payload.lane[0] = seed + mode * 4;
        packet.payload.lane[1] = seed * 2 - mode;
    }
    return packet;
}

static Wave48Packet choose_packet(Wave48Packet current, int step) {
    switch ((current.tag + current.tail + step) & 3) {
        case 0:
            return make_packet(step + 4, current.tag & 3);
        case 1:
            current.payload.edge.left = current.tail + step;
            current.payload.edge.right = current.tag - step;
            current.tail += current.payload.edge.left;
            return current;
        case 2:
            return (Wave48Packet){step + current.tag, {{current.tail - step, current.tag + step}}, current.tail ^ step};
        default:
            current.payload.lane[0] += step * 2;
            current.payload.lane[1] -= current.tag;
            current.tail -= step;
            return current;
    }
}

static int score(Wave48Packet packet, int mode) {
    int total = packet.tag * 7 + packet.tail * 3;
    if (mode & 1) {
        total += packet.payload.edge.left * 5 - packet.payload.edge.right * 11;
    } else {
        total += packet.payload.lane[0] * 13 - packet.payload.lane[1] * 17;
    }
    return total;
}

int main(void) {
    Wave48Packet current = make_packet(5, 0);
    int mode = 0;
    int total = 0;
    int i;

    for (i = 0; i < 10; ++i) {
        Wave48Packet next = choose_packet(current, i);
        mode = (mode + i + next.tag) & 1;
        current = ((score(next, mode) + total) & 1) ? next : choose_packet(next, i + 2);
        total += score(current, mode);
    }

    printf("%d %d %d %d\n", current.tag, current.tail, score(current, mode), total);
    return 0;
}
