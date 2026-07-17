#include <stddef.h>
#include <stdio.h>

struct Field {
    unsigned char code;
    unsigned char payload[4];
};

union Item {
    struct Field field;
    unsigned char raw[5];
};

struct Store {
    union Item items[3];
    unsigned char tail;
};

static unsigned checksum(const struct Store *store) {
    unsigned acc = store->tail;

    for (int i = 0; i < 3; ++i) {
        acc = acc * 47u + store->items[i].raw[0];
        acc = acc * 29u + store->items[i].raw[2];
        acc = acc * 19u + store->items[i].raw[4];
    }

    return acc;
}

int main(void) {
    struct Store store = {
        .items[0] = (union Item){ .raw = { 1, 2, 3, 4 } },
        .items[1] = (union Item){ .field = { .code = 5, .payload = { [2] = 7 } } },
        .items[2].field.payload[0] = 11,
        .tail = 13,
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Store, items),
           (unsigned)offsetof(struct Store, tail),
           (unsigned)store.items[0].raw[4],
           (unsigned)store.items[1].raw[2],
           (unsigned)store.items[2].raw[0],
           checksum(&store));
    return 0;
}
