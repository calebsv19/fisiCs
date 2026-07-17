#include <stdio.h>

typedef union {
    struct {
        int lane[3];
        int bias;
    } vec;
    struct {
        int lo;
        int hi;
        int extra;
    } span;
} Wave41Payload;

typedef struct {
    int kind;
    Wave41Payload payload;
    int tail[2];
} Wave41Packet;

static Wave41Packet make_vec(int seed) {
    Wave41Packet p;
    p.kind = 1;
    p.payload.vec.lane[0] = seed + 2;
    p.payload.vec.lane[1] = seed * 2 + 1;
    p.payload.vec.lane[2] = seed * 3 - 1;
    p.payload.vec.bias = seed - 4;
    p.tail[0] = seed + 30;
    p.tail[1] = seed + 40;
    return p;
}

static Wave41Packet make_span(int seed) {
    Wave41Packet p;
    p.kind = 2;
    p.payload.span.lo = seed - 3;
    p.payload.span.hi = seed + 9;
    p.payload.span.extra = seed * 4;
    p.tail[0] = seed + 50;
    p.tail[1] = seed + 60;
    return p;
}

static int score_packet(Wave41Packet p) {
    if (p.kind == 1) {
        return p.payload.vec.lane[0] * 3 + p.payload.vec.lane[1] * 5 -
               p.payload.vec.lane[2] + p.payload.vec.bias + p.tail[0] -
               p.tail[1];
    }
    return p.payload.span.lo * 7 + p.payload.span.hi * 2 +
           p.payload.span.extra - p.tail[0] + p.tail[1];
}

int main(void) {
    Wave41Packet current = make_vec(4);
    int total = score_packet(current);
    int i;

    for (i = 0; i < 7; ++i) {
        Wave41Packet candidate;
        switch ((i + current.kind + current.tail[0]) % 3) {
            case 0:
                candidate = make_span(i + 5);
                break;
            case 1:
                candidate = make_vec(i + 6);
                break;
            default:
                candidate = (score_packet(current) & 1)
                    ? make_span(i + current.kind)
                    : make_vec(i + current.kind + 2);
                break;
        }

        if (score_packet(candidate) > score_packet(current) + i) {
            current = candidate;
        } else {
            current.tail[i & 1] += i + current.kind;
        }
        total += score_packet(current);
    }

    printf("%d %d %d %d\n", current.kind, current.tail[0], current.tail[1], total);
    return 0;
}
