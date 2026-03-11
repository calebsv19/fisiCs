int main(void) {
    int a = 0;
    int b = 0;
    int c = 1;
    int v = a ? 1 : b ? 2 : c ? 3 : 4;
    return v == 3 ? 0 : 1;
}
