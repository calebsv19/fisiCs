#line 6601 "virtual_types_include_nested_designator_array_nonconst_probe.h"
int probe_wave50_include_nested_nonconst_index = 1;
int probe_wave50_include_nested_nonconst_grid[2][2] = {[1] = {[probe_wave50_include_nested_nonconst_index] = 1}};

static int probe_wave50_include_nested_designator_array_nonconst(void) {
    return probe_wave50_include_nested_nonconst_grid[1][1];
}
