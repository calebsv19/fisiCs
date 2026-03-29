#include <stdio.h>

extern int g_init_order_counter;
int init_order_seed(void);
int init_order_step(int delta);
int init_order_mix(int factor);
int init_order_snapshot(void);

int main(void) {
    int a = init_order_seed();
    int b = init_order_step(4);
    int c = init_order_mix(3);
    int d = init_order_step(-2);
    int e = init_order_mix(5);
    int f = init_order_snapshot();

    printf("%d %d %d %d %d %d %d\n", a, b, c, d, e, f, g_init_order_counter);
    return 0;
}
