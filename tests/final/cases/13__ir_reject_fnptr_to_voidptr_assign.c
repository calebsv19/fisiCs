int add1(int x) {
    return x + 1;
}

int main(void) {
    int (*fp)(int) = add1;
    void* p;
    p = fp;
    return 0;
}
