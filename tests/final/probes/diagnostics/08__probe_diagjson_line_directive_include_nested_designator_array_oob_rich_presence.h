#line 4301 "virtual_init_include_nested_designator_array_oob_probe.h"
int grid[2][2] = {
    [1] = {
        [3] = 1
    }
};

static int probe_include_nested_array_oob(void) {
    return grid[0][0];
}
