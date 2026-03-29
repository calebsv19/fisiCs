static int wave80_hidden_seed = 4;

int wave80_step(void) {
    static int wave80_local = 9;
    wave80_hidden_seed += 3;
    wave80_local += wave80_hidden_seed;
    return wave80_local;
}

int wave80_hidden(void) {
    return wave80_hidden_seed;
}
