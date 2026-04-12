#line 12701 "virtual_scope_extern_array_incomplete_then_definition_diag_clean.c"
extern int arr_inc_complete_clean[];
int arr_inc_complete_clean[4] = {1, 2, 3, 4};

int main(void) {
    return arr_inc_complete_clean[2];
}
