#include <stdio.h>

typedef int (*StatefulCbFn)(int *, int);

StatefulCbFn fnptr_reseed_state_pick(int seed, int *cursor);

int main(void) {
    int cursor = 2;
    int total = 0;
    total += fnptr_reseed_state_pick(3, &cursor)(&cursor, 4);
    total += fnptr_reseed_state_pick(6, &cursor)(&cursor, 2);
    if (cursor != 10) return 1;
    if (total != 25) return 2;
    printf("%d %d\n", total, cursor);
    return 0;
}
