typedef int (*StateCallback)(int *, int);

int fnptr_callback_state_fold(int *state, int count, StateCallback cb) {
    int total = 0;
    int i;
    for (i = 0; i < count; ++i) {
        total += cb(state, i);
    }
    return total;
}
