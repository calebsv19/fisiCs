#line 5821 "virtual_stmt_switch_duplicate_case_enum_alias_probe.c"
enum AliasCaseProbe {
    ALIAS_CASE_ZERO = 0,
    ALIAS_CASE_ONE = 1,
    ALIAS_CASE_ONE_DUP = 1
};

int main(void) {
    int x = 1;
    switch (x) {
        case ALIAS_CASE_ONE:
            return 0;
        case ALIAS_CASE_ONE_DUP:
            return 1;
        default:
            return 2;
    }
}
