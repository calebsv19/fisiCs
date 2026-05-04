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

struct TablePack {
    struct Table tables[2];
};

static struct TablePack pack = {
    .tables[0].cells[0][0][0] = (struct Node){ &a, &b },
    .tables[0].cells[0][0][1] = (struct Node){ &c, &d },
    .tables[0].cells[0][1][0] = (struct Node){ &e, &a },
    .tables[0].cells[0][1][1] = (struct Node){ &b, &c },
    .tables[0].cells[1][1][0] = (struct Node){ &d, &e },
    .tables[0].edges[0][0] = &a,
    .tables[0].edges[0][1] = &b,
    .tables[0].edges[1][0] = &c,
    .tables[0].edges[1][1] = &d,
    .tables[1].cells[1][0][0] = (struct Node){ &e, &d },
    .tables[1].cells[1][0][1] = (struct Node){ &c, &b },
    .tables[1].cells[1][1][0] = (struct Node){ &a, &e },
    .tables[1].cells[1][1][1] = (struct Node){ &b, &d },
    .tables[1].cells[0][1][1] = (struct Node){ &c, &a },
    .tables[1].edges[0][0] = &d,
    .tables[1].edges[0][1] = &c,
    .tables[1].edges[1][0] = &b,
    .tables[1].edges[1][1] = &e,
};

static unsigned checksum(void) {
    unsigned acc = 0;
    for (int t = 0; t < 2; ++t) {
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const struct Node *n = &pack.tables[t].cells[x][y][z];
                    int lhs = n->lhs ? *n->lhs : 0;
                    int rhs = n->rhs ? *n->rhs : 0;
                    int edge = pack.tables[t].edges[y][z] ? *pack.tables[t].edges[y][z] : 0;
                    acc = acc * 59u + (unsigned)(lhs * 11 + rhs * 7 + edge * 3 + t + x + y + z);
                }
            }
        }
    }
    return acc;
}

int main(void) {
    printf("%d %d %d %u\n",
           pack.tables[0].cells[0][1][0].lhs ? *pack.tables[0].cells[0][1][0].lhs : 0,
           pack.tables[1].edges[1][1] ? *pack.tables[1].edges[1][1] : 0,
           pack.tables[1].cells[0][1][1].rhs ? *pack.tables[1].cells[0][1][1].rhs : 0,
           checksum());
    return 0;
}
