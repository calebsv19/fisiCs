#include "12__probe_wave34_nested_include_initializer_hole_then_type_followup_a.h"

int main(void) {
    struct wave34_nested_box box = { .value = wave34_nested_initializer_value };
    int *ptr = &box;
    return *ptr;
}
