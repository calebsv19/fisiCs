#line 6701 "virtual_init_include_nested_designator_array_negative_probe_diag_text.h"
int grid_probe_include_nested_negative_text[2][2] = {[1] = {[-1] = 1}};

static int probe_diag_text_include_nested_designator_array_negative(void) {
    return grid_probe_include_nested_negative_text[1][0];
}
