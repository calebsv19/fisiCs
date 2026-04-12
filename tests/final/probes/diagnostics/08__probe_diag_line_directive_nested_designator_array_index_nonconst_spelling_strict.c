#line 6401 "virtual_init_nested_designator_array_nonconst_probe_diag_text.c"
int idx_probe_nested_nonconst_text = 1;
int grid_probe_nested_nonconst_text[2][2] = {[1] = {[idx_probe_nested_nonconst_text] = 1}};

int main(void) {
    return grid_probe_nested_nonconst_text[1][1];
}
