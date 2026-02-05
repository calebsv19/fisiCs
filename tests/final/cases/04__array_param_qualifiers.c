int takes_const(int a[const 4]) {
    a = 0;
    return 0;
}

int takes_static(int b[static 4]) {
    return b[0];
}

int main(void) {
    int small[2] = {0};
    int big[4] = {0};
    takes_static(big);
    takes_static(small);
    return takes_const(big);
}
