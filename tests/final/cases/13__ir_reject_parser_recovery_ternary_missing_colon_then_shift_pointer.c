int main(void) {
    int keep = 1;
    int broken = keep ? 2 3;
    int value = 4;
    int* p = &value;
    return (int)(p << 1);
}
