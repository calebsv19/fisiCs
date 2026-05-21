#line 3501 "virtual_init_include_designator_array_oob_probe.h"
int arr[2] = {[3] = 1};

static int probe_include_designator_array_oob(void) {
    return arr[0];
}
