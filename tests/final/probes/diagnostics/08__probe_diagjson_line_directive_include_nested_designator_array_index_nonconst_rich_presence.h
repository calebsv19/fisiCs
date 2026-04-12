#line 6501 "virtual_init_include_nested_designator_array_nonconst_probe_header.h"
int idx_include_nested_nonconst_probe = 1;
int grid_include_nested_nonconst_probe[2][2] = {[1] = {[idx_include_nested_nonconst_probe] = 1}};

static int wave9_probe_include_nested_nonconst(void) {
    return grid_include_nested_nonconst_probe[1][1];
}
