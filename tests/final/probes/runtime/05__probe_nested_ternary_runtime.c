int main(void) {
    int a = 0;
    int b = 1;
    int c = 0;
    int v = a ? 1 : b ? 2 : c ? 3 : 4;
    return v == 2 ? 0 : 1;
}
