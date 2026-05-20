typedef int (*BridgeCallback)(int, int);

int fnptr_callback_accumulate(int seed, int count, BridgeCallback cb) {
    int total = 0;
    int i;
    for (i = 0; i < count; ++i) {
        total += cb(seed, i);
    }
    return total;
}
