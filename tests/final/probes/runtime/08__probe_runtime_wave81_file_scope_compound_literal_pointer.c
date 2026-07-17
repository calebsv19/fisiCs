#include <stddef.h>
#include <stdio.h>

union Payload {
    unsigned char bytes[4];
    struct {
        unsigned char left;
        unsigned char right;
        unsigned char aux;
        unsigned char mark;
    } fields;
};

struct Entry {
    unsigned char tag;
    union Payload payload;
};

struct Frame {
    struct Entry entries[2];
    unsigned char seal;
};

static const struct Frame *const frame = &(struct Frame){
    .entries[0] = { .tag = 3, .payload.fields = { .left = 5, .right = 7, .mark = 11 } },
    .entries[1].payload.bytes = { [1] = 13, [3] = 17 },
    .entries[1].tag = 19,
    .seal = 23,
};

int main(void) {
    printf("%u %u %u %u %u\n",
           (unsigned)offsetof(struct Frame, seal),
           (unsigned)frame->entries[0].payload.bytes[2],
           (unsigned)frame->entries[1].payload.bytes[1],
           (unsigned)frame->entries[1].payload.bytes[3],
           (unsigned)frame->seal);
    return 0;
}
