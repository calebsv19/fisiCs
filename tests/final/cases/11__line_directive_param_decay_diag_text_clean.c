#line 12101 "virtual_fn_param_decay_diag_text_clean.c"
int takes_arr_text(int a[]) { return a[0]; }
int takes_fn_text(int (*fn)(int)) { return fn(1); }
int inc_text(int x) { return x + 1; }

int main(void) {
    int arr[2] = {1, 2};
    return takes_arr_text(arr) + takes_fn_text(inc_text);
}
