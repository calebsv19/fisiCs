static int probe_increment(int value) {
    return value + 1;
}

int probe_dispatch(int (*callback)());

int probe_dispatch(int (*callback)(int value)) {
    return callback(41);
}

int main(void) {
    return probe_dispatch(probe_increment) == 42 ? 0 : 1;
}
