__attribute__((weak)) int weak_symbol_pick(void) {
    return 11;
}

int weak_entry(void) {
    return weak_symbol_pick();
}
