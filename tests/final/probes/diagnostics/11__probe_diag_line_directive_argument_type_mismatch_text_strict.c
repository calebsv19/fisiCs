#line 9101 "virtual_fn_argument_type_mismatch_probe_diag_text.c"
int takes_ptr_text(int *p) {
    return *p;
}

int main(void) {
    return takes_ptr_text(1);
}
