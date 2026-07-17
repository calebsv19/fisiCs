#include <stdio.h>

typedef struct Sample {
    unsigned int id;
    unsigned int values[3];
    unsigned int flags;
} Sample;

static unsigned int checksum(const Sample *sample) {
    return sample->id * 3u + sample->values[0] * 5u +
           sample->values[1] * 7u + sample->values[2] * 11u +
           sample->flags * 13u;
}

int main(void) {
    Sample positional = {17u, {4u, 19u, 28u}, 9u};
    Sample designated = {
        .flags = 9u,
        .values = {[2] = 28u, [0] = 4u, [1] = 19u},
        .id = 17u
    };
    unsigned int a = checksum(&positional);
    unsigned int b = checksum(&designated);

    printf("%u %u %u\n", a, b, a == b);
    return 0;
}
