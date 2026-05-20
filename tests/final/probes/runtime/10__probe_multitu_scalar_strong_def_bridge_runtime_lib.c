extern int bucket10_scalar_bridge;

int bucket10_scalar_bridge_score(void) {
    return bucket10_scalar_bridge * 2 + 1;
}

void bucket10_scalar_bridge_shift(int delta) {
    bucket10_scalar_bridge += delta * 3;
}
