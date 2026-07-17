#include <stdio.h>

enum State {
    STATE_LOW = -6,
    STATE_HIGH = 18
};

struct Payload {
    unsigned char raw;
    signed char delta;
    enum State state;
};

struct Envelope {
    struct Payload slots[2];
    unsigned short bias;
};

int main(void) {
    struct Envelope envelope = {
        .slots = {
            {.raw = 250u, .delta = -7, .state = STATE_LOW},
            {.raw = 19u, .delta = 11, .state = STATE_HIGH}
        },
        .bias = 22u
    };
    struct Payload *chosen = &envelope.slots[(unsigned int)envelope.bias & 1u];
    int promoted = (int)chosen->raw + (int)chosen->delta + (int)chosen->state;
    unsigned char folded = (unsigned char)(promoted + (int)envelope.bias);

    printf("%d %u %d\n", promoted, (unsigned int)folded, chosen == &envelope.slots[0]);
    return 0;
}
