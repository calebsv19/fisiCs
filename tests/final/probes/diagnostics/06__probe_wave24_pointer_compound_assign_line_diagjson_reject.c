int probe_wave24_pointer_compound_assign_line_diagjson(void) {
    int values[4] = {0, 1, 2, 3};
    int *left = &values[0];
    int *right = &values[2];
#line 12179 "virtual_lv_wave24_pointer_compound_assign_diagjson.c"
    left += right;
    return *left;
}
