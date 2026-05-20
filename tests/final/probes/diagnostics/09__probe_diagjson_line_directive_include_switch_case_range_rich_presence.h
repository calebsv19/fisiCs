#line 5921 "virtual_stmt_switch_case_range_include_probe.h"
static int probe_include_switch_case_range_diagjson(void) {
    int x = 0;
    switch (x) {
        case 1 ... 3:
            return 0;
        default:
            return 1;
    }
}
