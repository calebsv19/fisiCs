typedef int (*wave22_cb_t)(int);

int wave22_multitu_apply_callbacks(wave22_cb_t *callbacks, int seed) {
    return callbacks[0](seed) + callbacks[1](seed);
}
