int main(void) {
    int a = 1;
    int b = 0;
    int c = 0;
    int v = a ? 1 : b ? 2 : c ? 3 : 4;
    return v == 1 ? 0 : 1;
}
