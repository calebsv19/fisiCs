static int mix_step(int state, int seed) {
    long long next = (long long)state * 17 + (long long)seed * 9 + 5;
    next %= 10007;
    if (next < 0) {
        next += 10007;
    }
    return (int)next;
}

int bump_lane(int seed) {
    static int state = 37;
    state = mix_step(state, seed);
    return state;
}

int fold_lane(int acc, int x) {
    int y = x % 97;
    return (acc * 5 + y * 3 + 11) % 100000;
}
