int g_tentative_counter;

int abi_tentative_peek(void) {
    return g_tentative_counter;
}

int abi_tentative_bump(int delta) {
    g_tentative_counter += delta;
    return g_tentative_counter;
}
