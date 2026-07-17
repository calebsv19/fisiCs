#include <stddef.h>
#include <stdio.h>

struct Flags {
    unsigned int lo : 3;
    unsigned int hi : 5;
};

union Payload {
    char name[4];
    struct Flags flags;
};

struct Slot {
    unsigned char kind;
    union Payload payload;
};

struct Table {
    struct Slot slots[3];
    unsigned char end;
};

static unsigned checksum(const struct Table *table) {
    unsigned acc = table->end;

    for (int i = 0; i < 3; ++i) {
        const struct Slot *slot = &table->slots[i];
        acc = acc * 37u + slot->kind;
        if (slot->kind == 1) {
            acc = acc * 29u + (unsigned char)slot->payload.name[0];
            acc = acc * 23u + (unsigned char)slot->payload.name[3];
        } else {
            acc = acc * 19u + slot->payload.flags.lo;
            acc = acc * 17u + slot->payload.flags.hi;
        }
    }

    return acc;
}

int main(void) {
    struct Table table = {
        .slots[0] = { .kind = 1, .payload.name = "ab" },
        .slots[1].payload.flags = { .lo = 5, .hi = 17 },
        .slots[1].kind = 2,
        .slots[2] = { .kind = 1, .payload.name = { 'x', 'y', 'z', 0 } },
        .end = 31,
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Table, end),
           (unsigned)table.slots[0].payload.name[2],
           (unsigned)table.slots[1].payload.flags.lo,
           (unsigned)table.slots[1].payload.flags.hi,
           (unsigned)table.slots[2].payload.name[3],
           checksum(&table));
    return 0;
}
