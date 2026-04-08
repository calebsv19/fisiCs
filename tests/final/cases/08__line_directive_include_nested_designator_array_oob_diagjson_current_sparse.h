#line 3951 "virtual_init_include_nested_designator_array_oob_header_case.h"
int grid[2][2] = {
    [1] = {
        [3] = 1
    }
};

static int wave4_include_nested_array_oob(void) {
    return grid[0][0];
}
