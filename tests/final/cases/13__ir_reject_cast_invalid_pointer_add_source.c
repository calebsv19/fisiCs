int main(void) {
    int a = 1;
    int b = 2;
    int* p = &a;
    int* q = &b;
    return (int)(p + q);
}
