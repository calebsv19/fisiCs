#line 6401 "virtual_types_include_nested_designator_array_negative_probe.h"
int probe_wave48_include_nested_negative_guard = 0;
int probe_wave48_include_nested_negative_grid[2][2] = {[1] = {[-1] = 1}};

static int probe_wave48_include_nested_designator_array_negative(void) {
    return probe_wave48_include_nested_negative_grid[1][1] + probe_wave48_include_nested_negative_guard;
}
