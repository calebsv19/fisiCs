#line 9211 "virtual_diagrec_include_mixed_bad_call_followup.h"
int callee(int x) {
    return x;
}

int f(void) {
    int v = callee(1;
    int* p = 0;
    double* q = 0;
    p = q;
    return v;
}
