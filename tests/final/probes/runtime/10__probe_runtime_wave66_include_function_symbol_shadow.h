#ifndef FISICS_TEST_WAVE66_INCLUDE_FUNCTION_SYMBOL_SHADOW_H
#define FISICS_TEST_WAVE66_INCLUDE_FUNCTION_SYMBOL_SHADOW_H

int bucket10_wave66_route_target(int value);
int bucket10_wave66_route_lib_call(int value);

static int bucket10_wave66_route_local(int value) {
    static int total = 1;
    total += value;
    return total;
}

#endif
