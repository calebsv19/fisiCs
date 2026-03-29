static int g_boot_base = 23;
int g_init_order_counter;

static int depth_walk(int n) {
    static int depth_state;
    if (n <= 0) {
        return depth_state + 1;
    }
    depth_state += n * 3 + 1;
    return depth_walk(n - 1) + n + depth_state;
}

static int boot_token(void) {
    static int booted;
    static int token;
    if (!booted) {
        token = g_boot_base * 17 + 5;
        g_init_order_counter = token % 211;
        booted = 1;
    }
    return token;
}

int init_order_seed(void) {
    return boot_token();
}

int init_order_step(int delta) {
    static int state;
    if (state == 0) {
        state = boot_token() + depth_walk(2);
    }
    state = (state * 7 + delta + g_init_order_counter) % 10007;
    g_init_order_counter = (g_init_order_counter + delta * 3 + 19) % 509;
    return state ^ g_init_order_counter;
}

int init_order_mix(int factor) {
    static int calls;
    int value = init_order_step(factor + calls);
    ++calls;
    return value + depth_walk((factor & 1) + 1) + calls;
}

int init_order_snapshot(void) {
    return boot_token() + g_init_order_counter + depth_walk(1);
}
