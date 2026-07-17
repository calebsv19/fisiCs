#include <stdio.h>

enum Op {
    OP_NEG = -3,
    OP_ADD = 5,
    OP_BIG = 130
};

struct Slot {
    enum Op op;
    unsigned char raw;
    signed char delta;
};

struct Frame {
    struct Slot slots[3];
    unsigned short bias;
};

static int score_slot(struct Frame *frame, int index) {
    struct Slot *slot = &frame->slots[index];
    unsigned char widened = (unsigned char)(slot->raw + (unsigned char)frame->bias);
    int promoted = slot->op + slot->delta;
    unsigned int blend = (unsigned int)widened + (unsigned int)(unsigned char)promoted;
    return (int)(blend & 255u) + (slot->op > 100 ? 11 : -7);
}

int main(void) {
    struct Frame frame = {
        {{OP_NEG, 250u, -4}, {OP_ADD, 19u, 7}, {OP_BIG, 201u, -8}},
        9u
    };

    int first = score_slot(&frame, 0);
    int second = score_slot(&frame, 1);
    int third = score_slot(&frame, 2);
    printf("%d %d %d %d\n", first, second, third, first + second + third);
    return 0;
}
