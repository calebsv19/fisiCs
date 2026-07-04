#line 2391 "virtual_lv_include_conditional_increment_diagjson_probe.h"
static int probe_lv_include_conditional_increment_diagjson(void) {
    int flag = 1;
    int left = 2;
    int right = 3;
    (flag ? left : right)++;
    return left + right;
}
