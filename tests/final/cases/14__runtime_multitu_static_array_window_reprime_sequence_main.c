#include <stdio.h>

void prime_window(int seed);
int window_sum(int start, int width);
int edge_mix(void);

int main(void) {
    prime_window(42);
    int a = window_sum(0, 2);

    prime_window(42);
    int b = window_sum(0, 2);

    prime_window(17);
    int c = window_sum(4, 3);

    printf("%d %d %d %d\n", a, b, c, edge_mix());
    return 0;
}
