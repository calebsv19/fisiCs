#line 2291 "virtual_lv_include_conditional_increment_diag_probe.h"
static int probe_lv_include_conditional_increment_diag(void) {
    int flag = 1;
    int left = 2;
    int right = 3;
    (flag ? left : right)++;
    return left + right;
}
