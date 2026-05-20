#include <stdio.h>

int bucket10_array_lane[4];

void bucket10_array_seed(int base);
int bucket10_array_mix(int idx, int delta);
int bucket10_array_sum(void);

int main(void) {
    int first;
    int second;
    int total;

    bucket10_array_seed(10);
    first = bucket10_array_mix(1, 5);
    second = bucket10_array_mix(3, -2);
    total = bucket10_array_sum();
    printf("%d %d %d\n", first, second, total);
    return 0;
}
