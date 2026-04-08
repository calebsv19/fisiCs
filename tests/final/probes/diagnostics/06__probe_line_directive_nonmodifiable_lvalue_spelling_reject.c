#line 1801 "virtual_lv_nonmodifiable_assign_diag_probe.c"
int probe_lv_nonmodifiable_assign_diag(void) {
    int x = 1;
    (x + 1) = x;
    return 0;
}
