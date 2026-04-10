#line 9301 "virtual_fn_return_type_mismatch_probe.c"
int *h_bad_probe(void) {
    return 1;
}

int main(void) {
    return h_bad_probe() != 0;
}
