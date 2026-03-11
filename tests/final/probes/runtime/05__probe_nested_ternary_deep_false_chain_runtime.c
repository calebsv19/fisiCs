int main(void) {
    int a = 0;
    int b = 0;
    int c = 0;
    int d = 1;
    int v = a ? 1 : b ? 2 : c ? 3 : d ? 4 : 5;
    return v == 4 ? 0 : 1;
}
