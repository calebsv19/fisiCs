#line 6701 "virtual_init_include_nested_designator_array_negative_header_case.h"
int guard_include_nested_negative = 0;
int grid_include_nested_negative[2][2] = {[1] = {[-1] = 1}};

static int wave9_include_nested_array_negative(void) {
    return grid_include_nested_negative[1][1] + guard_include_nested_negative;
}
