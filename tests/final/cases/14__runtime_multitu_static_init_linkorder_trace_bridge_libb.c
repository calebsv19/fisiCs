extern int g_wave93_trace_counter;

static int wave93_trace_b_state;

static int wave93_trace_b_boot(void) {
    static int booted;
    static int token;
    if (!booted) {
        token = 263;
        booted = 1;
    }
    return token;
}

void wave93_trace_b_reset(int seed) {
    int boot = wave93_trace_b_boot();
    wave93_trace_b_state = (seed * 17 + boot) % 2003;
    if (wave93_trace_b_state < 0) {
        wave93_trace_b_state += 2003;
    }
}

int wave93_trace_y(int value) {
    wave93_trace_b_state = (wave93_trace_b_state * 7 + value + g_wave93_trace_counter + wave93_trace_b_boot()) % 7001;
    if (wave93_trace_b_state < 0) {
        wave93_trace_b_state += 7001;
    }
    g_wave93_trace_counter = (g_wave93_trace_counter + wave93_trace_b_state + value) % 997;
    if (g_wave93_trace_counter < 0) {
        g_wave93_trace_counter += 997;
    }
    return wave93_trace_b_state - g_wave93_trace_counter;
}

int wave93_trace_b_peek(void) {
    return wave93_trace_b_state;
}
