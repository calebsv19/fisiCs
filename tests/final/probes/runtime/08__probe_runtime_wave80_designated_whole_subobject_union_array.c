#include <stddef.h>
#include <stdio.h>

union NodePayload {
    unsigned char bytes[4];
    struct {
        unsigned char lo;
        unsigned char hi;
        unsigned char aux;
        unsigned char mark;
    } fields;
};

struct Node {
    unsigned char kind;
    union NodePayload payload;
};

struct Graph {
    struct Node rows[2][2];
    unsigned char done;
};

static unsigned checksum(const struct Graph *graph) {
    unsigned acc = graph->done;

    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            const struct Node *node = &graph->rows[r][c];
            acc = acc * 53u + node->kind;
            acc = acc * 47u + node->payload.bytes[0];
            acc = acc * 43u + node->payload.bytes[1];
            acc = acc * 41u + node->payload.bytes[3];
        }
    }

    return acc;
}

int main(void) {
    struct Graph graph = {
        .rows[0][0].payload.fields = { .lo = 3, .hi = 5, .aux = 7, .mark = 11 },
        .rows[0][0] = { .kind = 13, .payload.bytes = { [0] = 17, [3] = 19 } },
        .rows[0][1] = { .kind = 23, .payload.fields = { .lo = 29, .hi = 31 } },
        .rows[1] = {
            [0] = { .kind = 37, .payload.bytes = { 41, 43, 47, 0 } },
        },
        .rows[1][0].payload = (union NodePayload){ .fields = { .lo = 53, .mark = 59 } },
        .done = 61,
    };

    graph.rows[1][1] = (struct Node){ .kind = 67, .payload = (union NodePayload){ .bytes = { [2] = 71 } } };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Graph, done),
           (unsigned)graph.rows[0][0].payload.bytes[1],
           (unsigned)graph.rows[0][1].payload.fields.hi,
           (unsigned)graph.rows[1][0].payload.bytes[3],
           (unsigned)graph.rows[1][1].payload.bytes[2],
           checksum(&graph));
    return 0;
}
