static int store[12];
static int seeded = 0;

int *load_store(int seed) {
    if (!seeded) {
        for (int i = 0; i < 12; ++i) {
            store[i] = i * 3 + 1;
        }
        seeded = 1;
    }

    for (int i = 0; i < 12; ++i) {
        int next = (store[i] * 7 + seed + i * 5) % 257;
        if (next < 0) {
            next += 257;
        }
        store[i] = next;
    }

    return store;
}

int checksum_store(int step) {
    int sum = 0;
    for (int i = 0; i < 12; i += step) {
        sum += store[i] * (i + 1);
    }
    return sum;
}
