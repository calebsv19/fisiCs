int inc(int x) {
    return x + 1;
}

int dec(int x) {
    return x - 1;
}

int main(void) {
    int pick = 1;
    int (*fp)(int) = pick ? inc : dec;
    return fp(4) != 5;
}
