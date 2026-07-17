#include <stdio.h>

int bucket10_wave62_shared[4];

void bucket10_wave62_seed(int base);
int bucket10_wave62_lib_mix(int step);
int bucket10_wave62_lib_shadow_again(int step);
int bucket10_wave62_aux_total(void);

static int bucket10_wave62_main_step(int step) {
    extern int bucket10_wave62_shared[];
    static int local = 5;

    local += step;
    bucket10_wave62_shared[1] += local;
    bucket10_wave62_shared[3] += step;
    {
        int first = bucket10_wave62_shared[1] - bucket10_wave62_shared[3];
        int second = local;
        int bucket10_wave62_shared[2] = {first, second};
        return bucket10_wave62_shared[0] + bucket10_wave62_shared[1] +
               step + local + 10;
    }
}

int main(void) {
    bucket10_wave62_seed(4);
    printf("%d %d %d %d %d\n",
           bucket10_wave62_lib_mix(3),
           bucket10_wave62_main_step(2),
           bucket10_wave62_aux_total(),
           bucket10_wave62_lib_shadow_again(1),
           bucket10_wave62_aux_total());
    return 0;
}
