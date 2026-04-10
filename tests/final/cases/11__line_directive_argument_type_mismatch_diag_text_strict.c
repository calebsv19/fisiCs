#line 9101 "virtual_fn_argument_type_mismatch_diag_text_strict.c"
int takes_ptr_text(int *p) {
    return *p;
}

int main(void) {
    return takes_ptr_text(1);
}
