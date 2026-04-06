#include <stdio.h>

void prime_window(int seed);
int window_sum(int start, int width);
int edge_mix(void);

int main(void) {
    prime_window(-13);
    int a = window_sum(0, 4);

    prime_window(3);
    int b = window_sum(3, 4);

    int c = edge_mix();
    printf("%d %d %d\n", a, b, c);
    return 0;
}
