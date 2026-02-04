int main(void) {
    int x = 1;
    int *p = &x;
    int y = (int)*p;
    return y;
}
