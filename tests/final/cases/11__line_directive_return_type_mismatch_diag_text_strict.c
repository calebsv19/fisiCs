#line 9701 "virtual_fn_return_type_mismatch_diag_text_strict.c"
int *h_bad_text_case(void) {
    return 1;
}

int main(void) {
    return h_bad_text_case() != 0;
}
