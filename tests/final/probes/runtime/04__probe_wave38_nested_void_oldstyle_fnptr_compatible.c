static int probe_answer(void) {
    return 42;
}

int probe_dispatch_void(int (*callback)(void));

int probe_dispatch_void(int (*callback)()) {
    return callback();
}

int main(void) {
    return probe_dispatch_void(probe_answer) == 42 ? 0 : 1;
}
