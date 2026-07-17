#include <stddef.h>
#include <stdio.h>

struct Member {
    unsigned char id;
    unsigned char slot[2][2];
};

union Bucket {
    struct Member member;
    unsigned char raw[5];
};

struct Shelf {
    union Bucket buckets[2][2];
};

static unsigned checksum(const struct Shelf *shelf) {
    unsigned acc = 0;

    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            const union Bucket *bucket = &shelf->buckets[r][c];
            acc = acc * 41u + bucket->raw[0];
            acc = acc * 23u + bucket->raw[2];
            acc = acc * 11u + bucket->raw[4];
        }
    }

    return acc;
}

int main(void) {
    struct Shelf shelf = {
        .buckets[0][0].member = {
            .id = 3,
            .slot = {
                [0] = { 5, 7 },
                [1][1] = 11,
            },
        },
        .buckets[0][1].raw = { 13, 17, 19 },
        .buckets[1][0] = (union Bucket){ .member = { .id = 23, .slot = { [1][0] = 29 } } },
        .buckets[1][1].member.slot[0][1] = 31,
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Shelf, buckets),
           (unsigned)sizeof(shelf.buckets[0][0]),
           (unsigned)shelf.buckets[0][0].raw[4],
           (unsigned)shelf.buckets[0][1].raw[3],
           (unsigned)shelf.buckets[1][1].raw[2],
           checksum(&shelf));
    return 0;
}
