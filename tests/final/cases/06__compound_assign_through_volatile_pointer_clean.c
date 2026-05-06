int main(void) {
    volatile int value = 4;
    volatile int *p = &value;
    *p += 6;
    *p -= 2;
    return value;
}
