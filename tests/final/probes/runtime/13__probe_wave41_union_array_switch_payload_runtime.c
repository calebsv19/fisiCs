#include <stdio.h>

typedef union {
    int scalar[4];
    struct {
        int id;
        int value;
        int guard[2];
    } record;
} Wave41Slot;

typedef struct {
    int mode;
    Wave41Slot slot;
    int trailer;
} Wave41Frame;

static Wave41Frame make_scalar(int base) {
    Wave41Frame f;
    f.mode = 1;
    f.slot.scalar[0] = base + 1;
    f.slot.scalar[1] = base + 3;
    f.slot.scalar[2] = base + 5;
    f.slot.scalar[3] = base + 7;
    f.trailer = base + 90;
    return f;
}

static Wave41Frame make_record(int base) {
    Wave41Frame f;
    f.mode = 2;
    f.slot.record.id = base - 2;
    f.slot.record.value = base * 6;
    f.slot.record.guard[0] = base + 11;
    f.slot.record.guard[1] = base + 13;
    f.trailer = base + 120;
    return f;
}

static int frame_score(Wave41Frame f) {
    if (f.mode == 1) {
        return f.slot.scalar[0] + f.slot.scalar[1] * 2 +
               f.slot.scalar[2] * 3 + f.slot.scalar[3] * 4 + f.trailer;
    }
    return f.slot.record.id * 5 + f.slot.record.value -
           f.slot.record.guard[0] + f.slot.record.guard[1] * 2 + f.trailer;
}

int main(void) {
    Wave41Frame frames[3];
    Wave41Frame carry = make_scalar(2);
    int total = 0;
    int i;

    frames[0] = make_scalar(5);
    frames[1] = make_record(6);
    frames[2] = make_scalar(8);

    for (i = 0; i < 6; ++i) {
        Wave41Frame chosen;
        switch ((i + carry.mode) & 3) {
            case 0:
                chosen = frames[0];
                break;
            case 1:
                chosen = make_record(i + 9);
                break;
            case 2:
                chosen = frames[(i + 1) % 3];
                break;
            default:
                chosen = (frame_score(carry) > frame_score(frames[i % 3]))
                    ? carry
                    : frames[i % 3];
                break;
        }

        carry = chosen;
        frames[i % 3] = carry;
        total += frame_score(carry);
    }

    printf("%d %d %d %d\n", carry.mode, carry.trailer, frame_score(carry), total);
    return 0;
}
