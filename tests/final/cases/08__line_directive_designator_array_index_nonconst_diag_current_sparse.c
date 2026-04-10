#line 4801 "virtual_init_designator_array_nonconst_diag_text_case.c"
int idx_nonconst_text = 1;
int arr_nonconst_text[2] = {[idx_nonconst_text] = 1};

int main(void) {
    return arr_nonconst_text[0];
}
