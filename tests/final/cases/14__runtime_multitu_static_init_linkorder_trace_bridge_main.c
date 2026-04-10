#include <stdio.h>

int wave93_trace_reset(int seed);
int wave93_trace_x(int value);
int wave93_trace_y(int value);
int wave93_trace_snapshot(void);

static int run_path(int seed, int start_with_y) {
    int p;
    int q;
    int r;
    int s;
    int total;

    (void) wave93_trace_reset(seed);
    if (start_with_y) {
        p = wave93_trace_y(seed + 3);
        q = wave93_trace_x(seed + 5);
    } else {
        p = wave93_trace_x(seed + 5);
        q = wave93_trace_y(seed + 3);
    }
    r = wave93_trace_x(seed - 2);
    s = wave93_trace_y(seed + 7);
    total = p + q * 3 + r * 5 + s * 7 + wave93_trace_snapshot();
    return total;
}

int main(void) {
    int a1 = run_path(21, 0);
    int a2 = run_path(21, 0);
    int b1 = run_path(21, 1);
    int b2 = run_path(21, 1);
    printf("%d %d %d %d\n", a1, a2, b1, b2);
    if (a1 != a2 || b1 != b2) {
        return 5;
    }
    return 0;
}
