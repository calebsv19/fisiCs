#line 5711 "virtual_stmt_include_switch_duplicate_case_folded_diagjson_probe.h"
static int probe_include_switch_duplicate_case_folded_diagjson(void) {
    int x = 1;
    switch (x) {
        case 1: return 0;
        case (1): return 1;
        default: return 2;
    }
}
