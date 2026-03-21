int f(int x) {
    return x;
}

int main(void) {
    int (*fp)(int, int);
    fp = f;
    return 0;
}
