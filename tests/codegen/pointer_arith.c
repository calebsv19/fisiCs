int main(void) {
    int buffer[4] = {1, 2, 3, 4};
    int *p = buffer;
    int *q = p + 2;
    int *r = q - 1;
    long int diff = q - r;

    if (p < q && q != r) {
        int *addr = &buffer[1];
        int value = *addr;
        ++p;
        r--;
        return (int)(diff + value + (p > r));
    }
    return 0;
}
