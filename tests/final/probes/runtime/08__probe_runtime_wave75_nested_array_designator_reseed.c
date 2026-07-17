#include <stddef.h>
#include <stdio.h>

struct Slot {
    unsigned char id;
    unsigned char bytes[4];
};

union Node {
    struct Slot slot;
    unsigned char raw[5];
};

struct Plane {
    union Node nodes[2][3];
};

struct Volume {
    struct Plane planes[2];
    unsigned char guard;
};

static unsigned checksum(const struct Volume *volume) {
    unsigned acc = volume->guard;

    for (int p = 0; p < 2; ++p) {
        const struct Plane *plane = &volume->planes[p];
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 3; ++c) {
                const union Node *node = &plane->nodes[r][c];
                acc = acc * 47u + node->raw[0];
                acc = acc * 31u + node->raw[1];
                acc = acc * 19u + node->raw[4];
            }
        }
    }

    return acc;
}

int main(void) {
    struct Volume volume = {
        .planes[0].nodes[0][2].slot = { .id = 3, .bytes = { 5, 7 } },
        .planes[0].nodes[1][0].raw = { 11, 13, 17, 19, 23 },
        .planes[0].nodes[1][0].slot.bytes[2] = 29,
        .planes[1] = {
            .nodes = {
                [0][1].slot = { .id = 31, .bytes = { [3] = 37 } },
                [1][2].raw = { 41, 43 },
            },
        },
        .planes[1].nodes[0][1].raw[4] = 47,
        .planes[1].nodes[1][2].slot = (struct Slot){ .id = 53, .bytes = { 59, 61 } },
        .guard = 67,
    };

    printf("%u %u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Volume, planes),
           (unsigned)offsetof(struct Plane, nodes),
           (unsigned)sizeof(volume.planes[0].nodes[0]),
           (unsigned)volume.planes[0].nodes[0][0].raw[0],
           (unsigned)volume.planes[0].nodes[1][0].raw[4],
           (unsigned)volume.planes[1].nodes[1][2].raw[0],
           checksum(&volume));
    return 0;
}
