#include <stdio.h>

enum SlotKind {
    SLOT_A = 2,
    SLOT_B = 6,
    SLOT_C = 11
};

struct Segment {
    enum SlotKind kind;
    unsigned char bytes[3];
    signed char trim;
};

struct Bucket {
    struct Segment segments[2];
    unsigned short seed;
};

static int checksum_bucket(struct Bucket *bucket, int selector) {
    struct Segment *segment = &bucket->segments[(unsigned int)(unsigned char)selector & 1u];
    unsigned char *bytes = segment->bytes;
    void *roundtrip = bytes;
    unsigned char *again = (unsigned char *)roundtrip;
    int index = ((int)segment->kind + (int)segment->trim + (int)(bucket->seed & 3u)) & 3;
    int picked = (int)again[index % 3];
    int neighbor = (int)bucket->segments[(index + 1) & 1].bytes[(index + 2) % 3];
    return picked + neighbor + (int)(unsigned char)(segment->trim + (signed char)segment->kind);
}

int main(void) {
    struct Bucket buckets[2] = {
        {{{SLOT_A, {250u, 17u, 91u}, -5}, {SLOT_C, {44u, 188u, 12u}, 9}}, 29u},
        {{{SLOT_B, {73u, 201u, 5u}, -11}, {SLOT_A, {136u, 22u, 240u}, 14}}, 46u}
    };

    int first = checksum_bucket(&buckets[0], 0);
    int second = checksum_bucket(&buckets[0], 1);
    int third = checksum_bucket(&buckets[1], 3);
    printf("%d %d %d %d\n", first, second, third, first + second + third);
    return 0;
}
