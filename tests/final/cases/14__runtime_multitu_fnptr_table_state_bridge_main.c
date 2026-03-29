#include <stdio.h>

typedef int (*op_fn)(int, int);

typedef struct OpLane {
    op_fn fn;
    int bias;
} OpLane;

extern OpLane select_lane(int mode);
extern int apply_lane(OpLane lane, int seed, int step);

int main(void) {
    int acc = 9;
    for (int i = 0; i < 8; ++i) {
        OpLane lane = select_lane(i);
        acc = acc * 17 + apply_lane(lane, acc + i, i);
    }
    printf("%d\n", acc);
    return 0;
}
