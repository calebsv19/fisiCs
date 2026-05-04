#include <stdio.h>

static int a = 1;
static int b = 4;
static int c = 9;
static int d = 16;
static int e = 25;

struct Node {
    int *lhs;
    int *rhs;
};

struct Table {
    struct Node cells[2][2][2];
    int *edges[2][2];
};

static struct Table tables[2] = {
    [0] = {
        .cells[0][0][0] = { &a, &b },
        .cells[0][0][1] = { &c, &d },
        .cells[0] = {
            [1] = {
                [0] = { &e, &a },
                [1] = { &b, &c },
            },
        },
        .cells[1][1][0] = { &d, &e },
        .edges = {
            { &a, &b },
            { &c, &d },
        },
    },
    [1] = {
        .cells[1] = {
            [0] = {
                [0] = { &e, &d },
                [1] = { &c, &b },
            },
            [1] = {
                [0] = { &a, &e },
                [1] = { &b, &d },
            },
        },
        .cells[0][1][1] = { &c, &a },
        .edges = {
            { &d, &c },
            { &b, &e },
        },
    },
};

static unsigned checksum(void) {
    unsigned acc = 0;

    for (int t = 0; t < 2; ++t) {
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const struct Node *n = &tables[t].cells[x][y][z];
                    int lhs = n->lhs ? *n->lhs : 0;
                    int rhs = n->rhs ? *n->rhs : 0;
                    int edge = tables[t].edges[y][z] ? *tables[t].edges[y][z] : 0;
                    acc = acc * 59u + (unsigned)(lhs * 11 + rhs * 7 + edge * 3 + t + x + y + z);
                }
            }
        }
    }
    return acc;
}

int main(void) {
    printf("%d %d %d %u\n",
           tables[0].cells[0][1][0].lhs ? *tables[0].cells[0][1][0].lhs : 0,
           tables[1].edges[1][1] ? *tables[1].edges[1][1] : 0,
           tables[1].cells[0][1][1].rhs ? *tables[1].cells[0][1][1].rhs : 0,
           checksum());
    return 0;
}
