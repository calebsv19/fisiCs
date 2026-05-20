struct Bucket10BridgeState {
    int base;
    int step;
    int bias;
};

extern struct Bucket10BridgeState bucket10_bridge_state;

int bucket10_bridge_score(void);
void bucket10_bridge_shift(int delta);
