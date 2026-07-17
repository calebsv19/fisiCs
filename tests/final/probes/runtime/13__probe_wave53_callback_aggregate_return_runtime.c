#include <stdio.h>

typedef struct {
    int left;
    int right;
    int stamp;
} Wave53Packet;

typedef Wave53Packet (*Wave53Transform)(Wave53Packet, int);

static Wave53Packet rotate(Wave53Packet packet, int step) {
    Wave53Packet out = {
        packet.right + step,
        packet.left - step,
        packet.stamp + packet.left - packet.right + step
    };
    return out;
}

static Wave53Packet dispatch(Wave53Transform transform, Wave53Packet packet, int step) {
    Wave53Packet copy = packet;
    copy.stamp += step * 3;
    return transform(copy, step + 1);
}

static int packet_score(Wave53Packet packet) {
    return packet.left * 5 - packet.right * 7 + packet.stamp * 11;
}

int main(void) {
    Wave53Packet packet = {3, 8, 13};
    int total = 0;
    int i;

    for (i = 0; i < 9; ++i) {
        packet = dispatch(rotate, packet, i);
        total += packet_score(packet);
    }

    printf("%d %d %d %d\n", packet.left, packet.right, packet.stamp, total);
    return 0;
}
