#include <stdio.h>

typedef union {
    struct {
        int lo;
        int hi;
    } span;
    int raw[2];
} Wave45Slot;

typedef struct {
    int kind;
    Wave45Slot slots[3];
    int carry;
} Wave45Frame;

static Wave45Frame make_frame(int kind, int seed) {
    Wave45Frame frame;
    int i;
    frame.kind = kind;
    frame.carry = seed + kind * 10;
    for (i = 0; i < 3; ++i) {
        if (kind == 1) {
            frame.slots[i].span.lo = seed + i;
            frame.slots[i].span.hi = seed * 2 - i;
        } else {
            frame.slots[i].raw[0] = seed * 3 + i * 2;
            frame.slots[i].raw[1] = seed - i * 4;
        }
    }
    return frame;
}

static int frame_score(Wave45Frame frame) {
    int total = frame.kind + frame.carry;
    int i;
    for (i = 0; i < 3; ++i) {
        if (frame.kind == 1) {
            total += frame.slots[i].span.lo * (i + 1) - frame.slots[i].span.hi;
        } else {
            total += frame.slots[i].raw[0] + frame.slots[i].raw[1] * (i + 2);
        }
    }
    return total;
}

int main(void) {
    Wave45Frame frame = make_frame(1, 6);
    int total = 0;
    int i;

    for (i = 0; i < 13; ++i) {
        Wave45Frame next = frame;
        switch ((frame_score(frame) + i) % 7) {
            case 0:
                next = make_frame(2, i + frame.kind + 4);
                break;
            case 1:
            case 2:
                next.slots[i % 3].raw[0] += frame.carry - i;
                next.carry += next.slots[(i + 1) % 3].raw[1];
                break;
            case 3:
                frame.carry += i + frame.kind;
                total += frame_score(frame);
                continue;
            case 4:
                next = (frame.kind == 1) ? make_frame(2, frame.carry % 11 + i) : make_frame(1, i + 3);
                next.slots[(i + 2) % 3].raw[1] -= frame.kind + i;
                break;
            default:
                next.carry -= i;
                break;
        }

        frame = (frame_score(next) >= frame_score(frame) - i) ? next : frame;
        total += frame_score(frame);
    }

    printf("%d %d %d %d %d\n", frame.kind, frame.carry, frame.slots[1].raw[0], frame_score(frame), total);
    return 0;
}
