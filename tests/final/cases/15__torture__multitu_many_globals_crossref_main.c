#include <stdio.h>

int g_main_seed = 7;
int g_main_bias[5] = {2, 4, 6, 8, 10};

int main_seed(void) {
    return g_main_seed;
}

int main_bias_at(int idx) {
    return g_main_bias[idx % 5];
}

int lib_mix(int idx);
int lib_window_sum(int span);

int main(void) {
    int acc = 0;

    for (int i = 0; i < 12; ++i) {
        acc += lib_mix(i);
        acc += lib_window_sum((i % 5) + 1);
    }

    printf("%d\n", acc);
    return 0;
}
