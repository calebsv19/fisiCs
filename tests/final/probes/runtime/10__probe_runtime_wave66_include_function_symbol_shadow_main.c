#include <stdio.h>

#include "10__probe_runtime_wave66_include_function_symbol_shadow.h"

static int bucket10_wave66_route_main_call(int value) {
    extern int bucket10_wave66_route_target(int value);

    int direct = bucket10_wave66_route_target(value);
    {
        int bucket10_wave66_route_target = 40;
        return direct + bucket10_wave66_route_target + bucket10_wave66_route_local(2);
    }
}

int main(void) {
    printf("%d %d\n",
           bucket10_wave66_route_lib_call(5),
           bucket10_wave66_route_main_call(6));
    return 0;
}
