#include <stdio.h>

int bucket10_array_lane[4];

void bucket10_array_seed(int base);
int bucket10_array_mix(int idx, int delta);
int bucket10_array_sum(void);

int main(void) {
    bucket10_array_seed(10);
    printf("%d %d %d\n",
           bucket10_array_mix(1, 5),
           bucket10_array_mix(3, -2),
           bucket10_array_sum());
    return 0;
}
