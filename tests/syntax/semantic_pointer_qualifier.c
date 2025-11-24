int main(void) {
    const int *src = 0;
    int *dst = 0;
    dst = src;  // should not drop const qualifier
    return *dst;
}
