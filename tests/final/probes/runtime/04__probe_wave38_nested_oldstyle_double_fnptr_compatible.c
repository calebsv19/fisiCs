static int probe_scale(double value) {
    return value == 4.5 ? 9 : 0;
}

int probe_dispatch_double(int (*callback)());

int probe_dispatch_double(int (*callback)(double value)) {
    return callback(4.5);
}

int main(void) {
    return probe_dispatch_double(probe_scale) == 9 ? 0 : 1;
}
