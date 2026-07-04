#line 2281 "virtual_lv_conditional_increment_diag_probe.c"
int probe_lv_conditional_increment_diag(void) {
    int flag = 1;
    int left = 2;
    int right = 3;
    (flag ? left : right)++;
    return left + right;
}
