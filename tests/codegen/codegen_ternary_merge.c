// Check that ternary merging pointer and void* results in void* in IR.
int main(void) {
    int x = 1;
    int val = 0;
    int* p = &val;
    void* v = x ? (void*)p : (void*)0;
    void* m = x ? (void*)0 : p;
    return (v == m);
}
