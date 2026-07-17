#include <stddef.h>
#include <stdio.h>

struct Point {
    unsigned char x;
    unsigned char y;
};

struct Shape {
    struct Point rows[2][2];
    unsigned char kind;
};

static const struct Shape shape = {
    .rows[0][0] = { 3, 5 },
    .rows[0][1].y = 7,
    .rows[1] = { { 11 }, { 13, 17 } },
    .kind = 19,
};

int main(void) {
    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Shape, kind),
           (unsigned)shape.rows[0][1].x,
           (unsigned)shape.rows[0][1].y,
           (unsigned)shape.rows[1][0].y,
           (unsigned)shape.rows[1][1].y,
           (unsigned)shape.kind);
    return 0;
}
