// Expect: no diagnostics
int main(void) {
    int a = 1;
    for (int i = 0; i < 2; ++i) {
        a += i;
    }
    return a;
}
