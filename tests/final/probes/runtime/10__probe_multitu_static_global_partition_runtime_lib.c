static int state = 30;

int read_local_state(void) {
    return state;
}

void bump_local_state(int delta) {
    state += delta;
}
