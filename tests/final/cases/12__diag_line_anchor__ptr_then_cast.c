struct S {
    int x;
};

int h(void) {
    int* p = 0;
    double* q = 0;
    p = q;
    return (int)(struct S){1};
}
