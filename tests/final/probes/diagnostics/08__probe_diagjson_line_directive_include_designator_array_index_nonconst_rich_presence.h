#line 4601 "virtual_init_include_designator_array_nonconst_probe.h"
int idx_probe_include_nonconst = 1;
int arr_probe_include_nonconst[2] = {[idx_probe_include_nonconst] = 1};

static int probe_include_designator_array_nonconst(void) {
    return arr_probe_include_nonconst[0];
}
