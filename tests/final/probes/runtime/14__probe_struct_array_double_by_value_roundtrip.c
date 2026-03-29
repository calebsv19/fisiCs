#include <stdio.h>

typedef struct Packet {
    int lane[3];
    double scale;
    short tag;
} Packet;

static Packet step(Packet p, int k) {
    Packet out = p;
    out.lane[0] = p.lane[0] + k;
    out.lane[1] = p.lane[1] - k;
    out.lane[2] = p.lane[2] + k * 2;
    out.scale = p.scale + (double)k * 0.25;
    out.tag = (short)(p.tag + k * 3);
    return out;
}

int main(void) {
    Packet a = {{3, 8, 13}, 1.5, 4};
    Packet b = step(a, 5);
    Packet c = step(b, -2);

    int sum_lane = c.lane[0] + c.lane[1] * 10 + c.lane[2] * 100;
    long long scaled = (long long)(c.scale * 100.0);

    printf("%d %lld %d\n", sum_lane, scaled, (int)c.tag);
    return 0;
}
