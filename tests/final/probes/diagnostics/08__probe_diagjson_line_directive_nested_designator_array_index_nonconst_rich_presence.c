#line 6401 "virtual_init_nested_designator_array_nonconst_probe.c"
int idx_nested_nonconst_probe = 1;
int grid_nested_nonconst_probe[2][2] = {[1] = {[idx_nested_nonconst_probe] = 1}};

int main(void) {
    return grid_nested_nonconst_probe[1][1];
}
