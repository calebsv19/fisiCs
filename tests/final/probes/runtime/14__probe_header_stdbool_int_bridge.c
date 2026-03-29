#include <stdbool.h>
#include <stdio.h>

static int bridge(bool lhs, bool rhs, int base) {
    int v = base;
    v += lhs ? 4 : -4;
    v += rhs ? 2 : -2;
    return v;
}

int main(void) {
    bool t = true;
    bool f = false;
    bool from_int = (bool)2;
    bool flip = (!t) || f;

    int lane_a = bridge(t, f, 10);
    int lane_b = bridge(from_int, flip, 3);

    printf("%d %d %d %d %d %d\n", (int)t, (int)f, (int)from_int, (int)flip, lane_a, lane_b);
    return 0;
}
