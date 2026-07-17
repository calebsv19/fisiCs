#include <stddef.h>
#include <stdio.h>

struct Node {
    unsigned char kind;
    union {
        struct {
            unsigned char x;
            unsigned char y;
            unsigned short sum;
        };
        unsigned char bytes[4];
    };
    struct {
        unsigned char lane;
        union {
            unsigned short code;
            struct {
                unsigned char lo;
                unsigned char hi;
            };
        };
    };
};

struct Scene {
    struct Node nodes[3];
    unsigned char footer[2];
};

static unsigned checksum(const struct Scene *scene) {
    unsigned acc = scene->footer[0] * 3u + scene->footer[1];

    for (int i = 0; i < 3; ++i) {
        const struct Node *node = &scene->nodes[i];
        acc = acc * 37u + node->kind;
        acc = acc * 31u + node->x;
        acc = acc * 29u + node->y;
        acc = acc * 23u + node->sum;
        acc = acc * 19u + node->lane;
        acc = acc * 17u + node->lo;
        acc = acc * 13u + node->hi;
        acc = acc * 11u + node->bytes[3];
    }

    return acc;
}

int main(void) {
    struct Scene scene = {
        .nodes[0] = {
            .kind = 2,
            .x = 3,
            .y = 5,
            .sum = 8,
            .lane = 13,
            .lo = 21,
            .hi = 34,
        },
        .nodes[1].bytes = { 55, 1, 2, 3 },
        .nodes[1].lane = 5,
        .nodes[1].code = 0x0807u,
        .nodes[2] = {
            .kind = 89,
            .bytes = { 6, 7, 8, 9 },
            .lane = 10,
            .code = 0x0c0bu,
        },
        .footer = { 13, 17 },
    };

    printf("%u %u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Scene, nodes),
           (unsigned)offsetof(struct Node, x),
           (unsigned)offsetof(struct Node, bytes),
           (unsigned)offsetof(struct Node, lo),
           (unsigned)scene.nodes[1].kind,
           (unsigned)scene.nodes[1].bytes[3],
           checksum(&scene));
    return 0;
}
