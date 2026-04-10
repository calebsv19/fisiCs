#line 9301 "virtual_fn_return_type_mismatch_case.c"
int *h_bad_case(void) {
    return 1;
}

int main(void) {
    return h_bad_case() != 0;
}
