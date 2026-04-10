#line 8701 "virtual_fn_argument_type_mismatch_case.c"
int takes_ptr(int *p) {
    return *p;
}

int main(void) {
    return takes_ptr(1);
}
