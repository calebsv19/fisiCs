int weak_symbol_pick(void) {
    return 7;
}

int strong_entry(void) {
    return weak_symbol_pick();
}
