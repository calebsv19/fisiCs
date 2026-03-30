#include <stdio.h>

typedef int (*OpFn)(int, int);

OpFn lib_pick_op(int row, int col);
int lib_seed(int row, int col);

int main(void) {
    int acc = 0;
    int x = 9;

    for (int r = 0; r < 6; ++r) {
        for (int c = 0; c < 7; ++c) {
            OpFn fn = lib_pick_op(r, c);
            int seed = lib_seed(r, c);
            int lhs = (x + r) % 257;
            int rhs = (seed + c) % 257;
            x = fn(lhs, rhs) % 257;
            if (x < 0) {
                x += 257;
            }
            acc += x + (r * 3) - c;
        }
    }

    printf("%d %d\n", acc, x);
    return 0;
}
