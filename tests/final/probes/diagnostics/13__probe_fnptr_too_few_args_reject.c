int f(int x) {
    return x;
}

int main(void) {
    int (*fp)(int) = f;
    return fp();
}
