#line 9201 "virtual_diagrec_mixed_bad_call_followup.c"
int callee(int x) {
    return x;
}

int main(void) {
    int v = callee(1;
    int* p = 0;
    double* q = 0;
    p = q;
    return v;
}
