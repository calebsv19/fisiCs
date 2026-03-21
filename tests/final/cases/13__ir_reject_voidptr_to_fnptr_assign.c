int add1(int x) {
    return x + 1;
}

int main(void) {
    void* p = 0;
    int (*fp)(int);
    fp = p;
    return 0;
}
