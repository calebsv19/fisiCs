#include <stdio.h>

#include "10__probe_runtime_wave66_include_extern_static_shadow.h"

int bucket10_wave66_shared_counter = 5;

static int bucket10_wave66_main_block_step(int delta) {
    extern int bucket10_wave66_shared_counter;

    bucket10_wave66_shared_counter += delta;
    {
        static int bucket10_wave66_shared_counter = 50;
        bucket10_wave66_shared_counter += delta;
        return bucket10_wave66_shared_counter;
    }
}

int main(void) {
    int lib_value = bucket10_wave66_include_lib_step(3);
    int header_value = bucket10_wave66_header_shadow_step(4);
    int block_value = bucket10_wave66_main_block_step(7);

    printf("%d %d %d %d\n",
           lib_value,
           header_value,
           block_value,
           bucket10_wave66_shared_counter);
    return 0;
}
