#line 5701 "virtual_stmt_switch_duplicate_case_folded_probe.c"
int main(void) {
    int x = 1;
    switch (x) {
        case 1: return 0;
        case (1): return 1;
        default: return 2;
    }
}
