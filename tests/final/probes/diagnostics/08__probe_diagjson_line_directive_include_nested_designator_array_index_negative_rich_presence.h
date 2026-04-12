#line 6701 "virtual_init_include_nested_designator_array_negative_probe_header.h"
int guard_include_nested_negative_probe = 0;
int grid_include_nested_negative_probe[2][2] = {[1] = {[-1] = 1}};

static int wave9_probe_include_nested_negative(void) {
    return grid_include_nested_negative_probe[1][1] + guard_include_nested_negative_probe;
}
