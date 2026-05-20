struct Bucket10HeaderStrongBridge {
    int base;
    int scale;
    int bias;
};

extern struct Bucket10HeaderStrongBridge bucket10_header_strong_bridge;

int bucket10_header_strong_bridge_score(void);
void bucket10_header_strong_bridge_shift(int delta);
