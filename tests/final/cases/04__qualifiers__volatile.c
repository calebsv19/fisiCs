volatile int g_volatile = 1;

int bump_volatile(void) {
    g_volatile = g_volatile + 1;
    return g_volatile;
}
