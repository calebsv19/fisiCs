int wave93_trace_b_peek(void);
void wave93_trace_b_reset(int seed);

int g_wave93_trace_counter;
static int wave93_trace_x_state;

static int wave93_trace_boot(void) {
    static int booted;
    static int token;
    if (!booted) {
        token = 157;
        booted = 1;
    }
    return token;
}

int wave93_trace_reset(int seed) {
    int boot = wave93_trace_boot();
    g_wave93_trace_counter = (seed + boot) % 911;
    if (g_wave93_trace_counter < 0) {
        g_wave93_trace_counter += 911;
    }
    wave93_trace_x_state = (seed * 13 + boot * 7) % 1009;
    if (wave93_trace_x_state < 0) {
        wave93_trace_x_state += 1009;
    }
    wave93_trace_b_reset(seed + boot);
    return wave93_trace_x_state;
}

int wave93_trace_x(int value) {
    wave93_trace_x_state = (wave93_trace_x_state * 5 + value + g_wave93_trace_counter * 3 + wave93_trace_boot()) % 5003;
    if (wave93_trace_x_state < 0) {
        wave93_trace_x_state += 5003;
    }
    g_wave93_trace_counter = (g_wave93_trace_counter + value * 2 + 11) % 997;
    if (g_wave93_trace_counter < 0) {
        g_wave93_trace_counter += 997;
    }
    return wave93_trace_x_state ^ g_wave93_trace_counter;
}

int wave93_trace_snapshot(void) {
    return wave93_trace_x_state + g_wave93_trace_counter + wave93_trace_b_peek();
}
