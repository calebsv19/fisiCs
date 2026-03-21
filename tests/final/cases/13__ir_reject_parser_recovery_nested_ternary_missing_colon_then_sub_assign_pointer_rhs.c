int main(void) {
    int a = 1;
    int b = 2;
    int c = 3;
    int broken = a ? (b ? 4 5) : c;
    int x = 3;
    int* p = 0;
    x -= p;
    return x;
}
