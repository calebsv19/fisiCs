#include <stddef.h>
#include <stdio.h>

union Item {
    unsigned char bytes[6];
    char text[6];
    struct {
        unsigned char tag;
        unsigned char value;
        unsigned char spare[4];
    } meta;
};

struct Bucket {
    union Item items[2][2];
    unsigned char mark;
};

struct Store {
    struct Bucket buckets[2];
};

static unsigned checksum(const struct Store *store) {
    unsigned acc = 0;

    for (int b = 0; b < 2; ++b) {
        const struct Bucket *bucket = &store->buckets[b];
        acc = acc * 47u + bucket->mark;
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                const union Item *item = &bucket->items[r][c];
                acc = acc * 23u + item->bytes[0];
                acc = acc * 19u + item->bytes[1];
                acc = acc * 17u + item->bytes[5];
            }
        }
    }

    return acc;
}

int main(void) {
    struct Store store = {
        .buckets[0].items[0][1].text = "ab",
        .buckets[0].items[0][1].bytes[5] = 7,
        .buckets[0].items[1][0].meta = {
            .tag = 11,
            .value = 13,
            .spare = { [3] = 17 },
        },
        .buckets[0].mark = 19,
        .buckets[1] = {
            .items = {
                [0][0].bytes = { 23, 29, 31 },
                [1][1].text = "xy",
            },
            .mark = 37,
        },
        .buckets[1].items[1][1].meta.value = 41,
    };

    printf("%u %u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Store, buckets),
           (unsigned)offsetof(struct Bucket, items),
           (unsigned)offsetof(struct Bucket, mark),
           (unsigned)store.buckets[0].items[0][0].bytes[0],
           (unsigned)store.buckets[0].items[0][1].bytes[2],
           (unsigned)store.buckets[1].items[1][1].bytes[0],
           checksum(&store));
    return 0;
}
