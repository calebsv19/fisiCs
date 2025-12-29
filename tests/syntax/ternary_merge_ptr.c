// Pointer/void*/null ternary merges should succeed without diagnostics.
int main(void) {
    int x = 1;
    void* vp = (x ? (void*)&x : 0);
    int* ip = (x ? (int*)0 : &x);
    void* mix = (x ? (void*)&x : ip);
    return vp == mix;
}
