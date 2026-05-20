struct Bucket10StrongBridge {
    int base;
    int scale;
    int bias;
};

extern struct Bucket10StrongBridge bucket10_strong_bridge;

int bucket10_strong_bridge_score(void) {
    return bucket10_strong_bridge.base * bucket10_strong_bridge.scale + bucket10_strong_bridge.bias;
}

void bucket10_strong_bridge_shift(int delta) {
    bucket10_strong_bridge.base += delta;
    bucket10_strong_bridge.scale += 1;
    bucket10_strong_bridge.bias -= delta;
}
