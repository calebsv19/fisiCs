#line 12301 "virtual_scope_extern_array_def_mismatch_diag_case.c"
extern int arr_def_diag[3];
int arr_def_diag[4] = {1, 2, 3, 4};

int main(void) {
    return arr_def_diag[0];
}
