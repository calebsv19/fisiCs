#line 11301 "virtual_scope_extern_array_consistent_diag_case.c"
extern int arr_clean[3];
int arr_clean[3] = {1, 2, 3};

int main(void) {
    return arr_clean[1];
}
