#include <stdio.h>

int bump_lane(int seed);
int fold_lane(int acc, int x);

int main(void) {
    int acc = 19;

    for (int i = 0; i < 8; ++i) {
        int seed = i * i + 3;
        int value = bump_lane(seed);
        acc = fold_lane(acc, value);
    }

    printf("%d\n", acc);
    return 0;
}
