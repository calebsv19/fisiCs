#include <stddef.h>
#include <stdio.h>

struct Header {
    unsigned char lane;
    unsigned char kind;
};

union Body {
    unsigned char bytes[5];
    struct {
        unsigned char x;
        unsigned char y;
        unsigned char z;
        unsigned char w;
        unsigned char q;
    } point;
};

struct Node {
    struct Header header;
    union Body bodies[2];
};

struct Scene {
    struct Node nodes[3];
    unsigned char trailer[2];
};

static unsigned checksum(const struct Scene *scene) {
    unsigned acc = scene->trailer[0] * 3u + scene->trailer[1];

    for (int n = 0; n < 3; ++n) {
        const struct Node *node = &scene->nodes[n];
        acc = acc * 41u + node->header.lane;
        acc = acc * 37u + node->header.kind;
        for (int b = 0; b < 2; ++b) {
            acc = acc * 19u + node->bodies[b].bytes[0];
            acc = acc * 17u + node->bodies[b].bytes[2];
            acc = acc * 13u + node->bodies[b].bytes[4];
        }
    }

    return acc;
}

int main(void) {
    struct Scene scene = {
        .nodes[0] = {
            .header = { .lane = 1, .kind = 2 },
            .bodies[0].bytes = { 3, 5, 7, 0, 0 },
            .bodies[1].point = { .x = 11, .z = 13, .q = 17 },
        },
        .nodes[1].bodies[1].bytes = { 19, 23 },
        .nodes[1].bodies[1].point.w = 29,
        .nodes[2].header.kind = 31,
        .nodes[2].bodies[0].point = { .x = 37, .y = 41, .q = 43 },
        .nodes[2].bodies[0].bytes[2] = 47,
        .trailer = { 53, 59 },
    };

    printf("%u %u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Scene, nodes),
           (unsigned)offsetof(struct Node, bodies),
           (unsigned)offsetof(union Body, point),
           (unsigned)scene.nodes[1].bodies[0].bytes[0],
           (unsigned)scene.nodes[1].bodies[1].bytes[2],
           (unsigned)scene.nodes[2].bodies[0].bytes[4],
           checksum(&scene));
    return 0;
}
