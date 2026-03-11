volatile int gv = 3;

int main(void) {
    volatile int *p = &gv;
    *p = *p + 1;
    return gv;
}
