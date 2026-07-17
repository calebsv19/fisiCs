#include "10__probe_runtime_wave66_include_function_symbol_shadow.h"

int bucket10_wave66_route_target(int value) {
    return (value * 2) + bucket10_wave66_route_local(3);
}

int bucket10_wave66_route_lib_call(int value) {
    return bucket10_wave66_route_target(value) + bucket10_wave66_route_local(value);
}
