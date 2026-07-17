#include <stdio.h>

typedef union {
    struct {
        int lane[3];
        int bias;
    } grid;
    struct {
        int lo;
        int hi;
        int mark;
        int pad;
    } span;
} Wave43BranchPayload;

typedef struct {
    int tag;
    Wave43BranchPayload payload;
    int tail;
} Wave43BranchPacket;

static int score(Wave43BranchPacket p) {
    if (p.tag == 1) {
        return p.payload.grid.lane[0] * 3 + p.payload.grid.lane[1] * 5 -
               p.payload.grid.lane[2] + p.payload.grid.bias + p.tail;
    }
    return p.payload.span.lo * 7 - p.payload.span.hi +
           p.payload.span.mark * 2 + p.payload.span.pad + p.tail;
}

int main(void) {
    Wave43BranchPacket current = (Wave43BranchPacket){1, {.grid = {{2, 4, 6}, 8}}, 10};
    int total = 0;
    int i;

    for (i = 0; i < 7; ++i) {
        Wave43BranchPacket next;
        if (((score(current) + i) & 3) == 0) {
            next = (Wave43BranchPacket){1, {.grid = {{i + 3, i + 5, i + 7}, i + 11}}, i + 20};
        } else if ((i & 1) == 0) {
            next = (Wave43BranchPacket){2, {.span = {i + 13, i + 2, i + 4, i + 6}}, i + 30};
        } else {
            next = current;
            next.tail += i + next.tag;
        }

        if (score(next) >= score(current) - i) {
            current = next;
        } else {
            current = (Wave43BranchPacket){1, {.grid = {{current.tag + i, current.tail - i, i + 9}, 3}}, i + 40};
        }
        total += score(current);
    }

    printf("%d %d %d %d\n", current.tag, current.tail, score(current), total);
    return 0;
}
