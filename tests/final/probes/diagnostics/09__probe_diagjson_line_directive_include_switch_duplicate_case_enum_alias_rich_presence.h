#line 5831 "virtual_stmt_include_switch_duplicate_case_enum_alias_probe.h"
enum AliasCaseIncludeProbe {
    ALIAS_INCLUDE_ZERO = 0,
    ALIAS_INCLUDE_ONE = 1,
    ALIAS_INCLUDE_ONE_DUP = 1
};

static int probe_include_switch_duplicate_case_enum_alias_diagjson(void) {
    int x = 1;
    switch (x) {
        case ALIAS_INCLUDE_ONE:
            return 0;
        case ALIAS_INCLUDE_ONE_DUP:
            return 1;
        default:
            return 2;
    }
}
