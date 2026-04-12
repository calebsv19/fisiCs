#line 6401 "virtual_init_nested_designator_array_nonconst_case.c"
int idx_nested_nonconst = 1;
int grid_nested_nonconst[2][2] = {[1] = {[idx_nested_nonconst] = 1}};

int main(void) {
    return grid_nested_nonconst[1][1];
}
