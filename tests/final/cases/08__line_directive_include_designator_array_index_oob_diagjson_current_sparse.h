#line 3301 "virtual_init_include_designator_array_oob_header_case.h"
int arr[2] = {[3] = 1};

static int wave3_include_designator_array_oob(void) {
    return arr[0];
}
