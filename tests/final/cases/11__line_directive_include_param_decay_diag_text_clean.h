#line 12201 "virtual_fn_include_param_decay_diag_text_clean.h"
int takes_arr_inc_text(int a[]) { return a[0]; }
int takes_fn_inc_text(int (*fn)(int)) { return fn(1); }
int inc_inc_text(int x) { return x + 1; }
int run_param_decay_inc_text(void) {
    int arr[2] = {1, 2};
    return takes_arr_inc_text(arr) + takes_fn_inc_text(inc_inc_text);
}
