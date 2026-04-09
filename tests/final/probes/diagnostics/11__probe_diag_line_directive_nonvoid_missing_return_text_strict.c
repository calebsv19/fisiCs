#line 7501 "virtual_fn_nonvoid_missing_return_probe_diag_text.c"
int f_text(int x) {
    if (x > 0) {
        return x;
    }
}

int main(void) {
    return f_text(1);
}
