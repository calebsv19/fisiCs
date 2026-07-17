static int wave27_rebind_const_pointer(int selected[const 4],
                                       int replacement[4]) {
#line 12701 "virtual_lv_wave27_bracket_const_parameter.c"
    selected = replacement;
    return selected[0];
}

int main(void) {
    int values[4] = {0, 1, 2, 3};
    return wave27_rebind_const_pointer(values, values);
}
