#line 6501 "virtual_init_include_nested_designator_array_nonconst_probe_diag_text.h"
int idx_probe_include_nested_nonconst_text = 1;
int grid_probe_include_nested_nonconst_text[2][2] = {[1] = {[idx_probe_include_nested_nonconst_text] = 1}};

static int probe_diag_text_include_nested_designator_array_nonconst(void) {
    return grid_probe_include_nested_nonconst_text[1][1];
}
