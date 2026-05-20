#line 5611 "virtual_stmt_include_switch_multiple_default_probe.h"
static int probe_include_switch_multiple_default(void) {
    int x = 2;
    switch (x) {
        default: return 0;
        default: return 1;
    }
}
