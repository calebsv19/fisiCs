#line 6501 "virtual_init_include_nested_designator_array_nonconst_header_case.h"
int idx_include_nested_nonconst = 1;
int grid_include_nested_nonconst[2][2] = {[1] = {[idx_include_nested_nonconst] = 1}};

static int wave9_include_nested_array_nonconst(void) {
    return grid_include_nested_nonconst[1][1];
}
