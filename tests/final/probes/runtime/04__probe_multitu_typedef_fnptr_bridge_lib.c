typedef int (*wave13_cb_t)(int);

int wave14_multitu_fnptr_bridge(wave13_cb_t cb, int seed) {
    return cb(seed);
}
