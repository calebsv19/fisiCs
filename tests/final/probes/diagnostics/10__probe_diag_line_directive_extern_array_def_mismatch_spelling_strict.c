#line 12901 "virtual_scope_extern_array_def_mismatch_probe_diag_text.c"
extern int arr_def_probe_diag[3];
int arr_def_probe_diag[4] = {1, 2, 3, 4};

int main(void) {
    return arr_def_probe_diag[0];
}
