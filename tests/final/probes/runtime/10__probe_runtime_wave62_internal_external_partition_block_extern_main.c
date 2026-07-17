#include <stdio.h>

int bucket10_wave62_counter = 7;

int bucket10_wave62_lib_tick(int step);
int bucket10_wave62_lib_read(void);
int bucket10_wave62_aux_touch(int step);

static int bucket10_wave62_main_fold(int step) {
    extern int bucket10_wave62_counter;
    static int local = 40;

    local += step;
    bucket10_wave62_counter += step + local / 10;
    {
        int bucket10_wave62_counter = local - step;
        return bucket10_wave62_counter + local + step + 11;
    }
}

int main(void) {
    printf("%d %d %d %d %d\n",
           bucket10_wave62_lib_tick(5),
           bucket10_wave62_main_fold(2),
           bucket10_wave62_aux_touch(4),
           bucket10_wave62_lib_read(),
           bucket10_wave62_counter);
    return 0;
}
