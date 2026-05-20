#include <stdio.h>

typedef int (*ProbeIntFnInc)(int);

ProbeIntFnInc probe_pick_transform_inc(int which);

int main(void) {
    ProbeIntFnInc a = probe_pick_transform_inc(0);
    ProbeIntFnInc b = probe_pick_transform_inc(1);
    int total = a(6) + b(2);
    printf("%d\n", total);
    return total == 19 ? 0 : 1;
}
