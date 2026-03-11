int apply_callback(int (*cb)(int), int x) {
    return cb(x);
}
