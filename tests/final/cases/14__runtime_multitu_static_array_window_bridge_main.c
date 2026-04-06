#include <stdio.h>

void prime_window(int seed);
int window_sum(int start, int width);
int edge_mix(void);

int main(void) {
    prime_window(5);
    int a = window_sum(1, 4);

    prime_window(11);
    int b = window_sum(2, 3);

    int c = edge_mix();
    printf("%d %d %d\n", a, b, c);
    return 0;
}
