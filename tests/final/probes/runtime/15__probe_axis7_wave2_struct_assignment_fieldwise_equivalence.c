#include <stdio.h>

typedef struct Frame {
    unsigned int tag;
    unsigned int samples[3];
    unsigned int tail;
} Frame;

static unsigned int checksum(const Frame *frame) {
    return frame->tag * 3u + frame->samples[0] * 5u + frame->samples[1] * 7u +
           frame->samples[2] * 11u + frame->tail * 13u;
}

int main(void) {
    Frame source = {9u, {4u, 12u, 25u}, 7u};
    Frame assigned = source;
    Frame fieldwise;
    fieldwise.tag = source.tag;
    fieldwise.samples[0] = source.samples[0];
    fieldwise.samples[1] = source.samples[1];
    fieldwise.samples[2] = source.samples[2];
    fieldwise.tail = source.tail;
    {
        unsigned int a = checksum(&assigned);
        unsigned int b = checksum(&fieldwise);
        printf("%u %u %u\n", a, b, a == b);
    }
    return 0;
}
