int a(int x) {
    return x;
}

int b(int x, int y) {
    return x + y;
}

int main(void) {
    int pick = 1;
    int (*fp)(int) = pick ? a : b;
    return fp(3);
}
