int f(void) { return 1; }
int* g(void) { return 0; }
int* h(void) { return 1; }
int main(void) {
    int* p = g();
    int* q = h();
    return (p != 0) + (q != 0) + f();
}
