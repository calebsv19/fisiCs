#include <stddef.h>
#include <stdio.h>

struct Flags {
    unsigned int mode : 3;
    unsigned int count : 5;
    unsigned int ready : 1;
};

struct Slot {
    unsigned char id;
    struct Flags flags;
};

struct Bank {
    struct Slot slots[3];
    unsigned char end;
};

static const struct Bank bank = {
    .slots[0].flags = { .mode = 5, .ready = 1 },
    .slots[0].id = 7,
    .slots[1] = { .id = 11, .flags = { .count = 17 } },
    .slots[2].flags.ready = 1,
    .end = 19,
};

int main(void) {
    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Bank, end),
           bank.slots[0].flags.mode,
           bank.slots[0].flags.count,
           bank.slots[1].flags.ready,
           bank.slots[2].flags.mode,
           bank.slots[2].flags.ready);
    return 0;
}
