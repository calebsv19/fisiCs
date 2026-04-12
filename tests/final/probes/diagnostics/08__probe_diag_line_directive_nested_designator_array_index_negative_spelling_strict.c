#line 6601 "virtual_init_nested_designator_array_negative_probe_diag_text.c"
int grid_probe_nested_negative_text[2][2] = {[1] = {[-1] = 1}};

int main(void) {
    return grid_probe_nested_negative_text[1][0];
}
