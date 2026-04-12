#line 12501 "virtual_fn_variadic_named_param_diag_text_clean.c"
int passthrough_sum(int base, ...) {
    return base;
}

int main(void) {
    return passthrough_sum(3, 7, 9);
}
