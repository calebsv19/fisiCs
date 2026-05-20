#line 5911 "virtual_stmt_switch_case_range_probe.c"
int main(void) {
    int x = 0;
    switch (x) {
        case 1 ... 3:
            return 0;
        default:
            return 1;
    }
}
