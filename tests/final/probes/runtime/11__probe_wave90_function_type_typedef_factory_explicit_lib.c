#include "11__probe_wave90_function_type_typedef_factory_contract.h"

int wave90_function_type_typedef_factory_call(
    int (*direct)(int),
    int (*(*chooser)(int))(int),
    int route,
    int value) {
    return direct(value) + chooser(route)(value);
}
