#line 4401 "virtual_init_designator_array_nonconst_case.c"
int idx_nonconst = 1;
int arr_nonconst[2] = {[idx_nonconst] = 1};

int main(void) {
    return arr_nonconst[0];
}
