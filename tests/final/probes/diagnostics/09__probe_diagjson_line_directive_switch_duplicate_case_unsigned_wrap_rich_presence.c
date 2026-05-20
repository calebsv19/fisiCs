#line 5861 "virtual_stmt_switch_duplicate_case_unsigned_wrap_probe.c"
int main(void) {
    unsigned int tag = 0u;
    switch (tag) {
        case -1:
            return 0;
        case 4294967295u:
            return 1;
        default:
            return 2;
    }
}
