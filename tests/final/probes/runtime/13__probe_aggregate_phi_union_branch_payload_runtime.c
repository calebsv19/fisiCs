#include <stdio.h>

typedef union {
    struct {
        int xy[2];
        int bias;
    } point;
    int lanes[3];
} NestedArm;

typedef struct {
    int tag;
    NestedArm arm;
    int tail[2];
} Payload;

static Payload make_point_payload(int seed) {
    Payload p;
    p.tag = 1;
    p.arm.point.xy[0] = seed + 3;
    p.arm.point.xy[1] = seed * 2 + 1;
    p.arm.point.bias = seed - 4;
    p.tail[0] = seed + 20;
    p.tail[1] = seed + 30;
    return p;
}

static Payload make_lane_payload(int seed) {
    Payload p;
    p.tag = 2;
    p.arm.lanes[0] = seed - 1;
    p.arm.lanes[1] = seed + 5;
    p.arm.lanes[2] = seed * 3;
    p.tail[0] = seed + 40;
    p.tail[1] = seed + 50;
    return p;
}

static Payload choose_payload(int flag, int seed) {
    Payload left = make_point_payload(seed);
    Payload right = make_lane_payload(seed + 2);

    if (flag) {
        return left;
    }
    return right;
}

static int read_payload(Payload p) {
    if (p.tag == 1) {
        return p.arm.point.xy[0] * 3 + p.arm.point.xy[1] - p.arm.point.bias +
               p.tail[0] + p.tail[1];
    }
    return p.arm.lanes[0] + p.arm.lanes[1] * 2 + p.arm.lanes[2] * 3 +
           p.tail[0] - p.tail[1];
}

int main(void) {
    Payload a = choose_payload(1, 5);
    Payload b = choose_payload(0, 7);
    Payload merged = read_payload(a) > read_payload(b) ? a : b;
    Payload copied;
    copied = merged;

    printf("%d %d %d %d\n", copied.tag, copied.tail[0], copied.tail[1],
           read_payload(copied));
    return 0;
}
