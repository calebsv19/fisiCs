int main(void) {
    int a = 1;
    int b = 0;
    int c = 1;
    int v = a ? (b ? 10 : (c ? 20 : 30)) : 40;
    return v == 20 ? 0 : 1;
}
