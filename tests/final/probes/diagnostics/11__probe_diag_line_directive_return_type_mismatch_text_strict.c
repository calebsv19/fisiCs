#line 10101 "virtual_fn_return_type_mismatch_probe_diag_text.c"
int *h_bad_text_probe(void) {
    return 1;
}

int main(void) {
    return h_bad_text_probe() != 0;
}
