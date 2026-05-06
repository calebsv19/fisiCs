static int adjust(volatile int *restrict p) {
    *p += 4;
    *p /= 2;
    return *p;
}

int main(void) {
    volatile int value = 8;
    return adjust(&value);
}
