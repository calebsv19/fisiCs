#line 2381 "virtual_lv_conditional_increment_diagjson_probe.c"
int probe_lv_conditional_increment_diagjson(void) {
    int flag = 1;
    int left = 2;
    int right = 3;
    (flag ? left : right)++;
    return left + right;
}
