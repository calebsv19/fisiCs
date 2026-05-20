#include <stdio.h>

typedef int (*ProbeIntFn)(int);

ProbeIntFn probe_pick_transform(int which);

int main(void) {
    ProbeIntFn a = probe_pick_transform(0);
    ProbeIntFn b = probe_pick_transform(1);
    int total = a(4) + b(5);
    if (total != 22) return 1;
    printf("%d\n", total);
    return 0;
}
