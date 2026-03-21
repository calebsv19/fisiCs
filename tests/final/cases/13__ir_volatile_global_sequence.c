volatile int g_counter;

int main(void) {
    volatile int* p = &g_counter;
    *p = 1;
    int a = *p;
    *p = a + 3;
    int b = *p;
    return b;
}
