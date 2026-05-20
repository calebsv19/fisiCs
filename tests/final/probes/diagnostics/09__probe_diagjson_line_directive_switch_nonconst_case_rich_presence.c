#line 5801 "virtual_stmt_switch_nonconst_case_probe.c"
int main(void) {
    int x = 2;
    int y = 1;
    switch (x) {
        case y:
            return 1;
        default:
            return 0;
    }
}
