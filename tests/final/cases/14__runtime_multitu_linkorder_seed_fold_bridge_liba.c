static int wave93_a_base = 41;
static int wave93_a_state[6];

static int wave93_mod(int value, int mod) {
    int out = value % mod;
    if (out < 0) {
        out += mod;
    }
    return out;
}

static int wave93_a_boot(void) {
    static int booted;
    static int token;
    if (!booted) {
        token = wave93_a_base * 13 + 7;
        booted = 1;
    }
    return token;
}

int wave93_a_reset(int seed) {
    int i;
    int boot = wave93_a_boot();
    for (i = 0; i < 6; ++i) {
        int lane = seed * (i + 3) + i * i * 11 + boot;
        wave93_a_state[i] = wave93_mod(lane, 9973);
    }
    return wave93_a_state[0] + wave93_a_state[5];
}

int wave93_a_step(int lane, int x) {
    int idx = wave93_mod(lane, 6);
    int next = wave93_mod(wave93_a_state[idx] * 5 + x + idx * 17 + wave93_a_boot(), 65521);
    int tail = wave93_a_state[(idx + 1) % 6];
    wave93_a_state[idx] = next;
    return next ^ (tail + idx * 13);
}

int wave93_a_peek(void) {
    int i;
    int sum = 0;
    for (i = 0; i < 6; ++i) {
        sum = wave93_mod(sum + wave93_a_state[i] * (i + 1), 100003);
    }
    return sum;
}
