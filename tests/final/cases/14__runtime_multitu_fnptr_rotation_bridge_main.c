#include <stdio.h>

typedef int (*LaneOp)(int, int);

LaneOp select_lane(int idx);
int reduce_state(int state, int salt);

int main(void) {
    int state = 21;

    for (int i = 0; i < 11; ++i) {
        int idx = (state + i) % 3;
        if (idx < 0) {
            idx += 3;
        }
        LaneOp op = select_lane(idx);
        int step = op(state, i + 2);
        state = reduce_state(step, idx + i);
    }

    printf("%d\n", state);
    return 0;
}
