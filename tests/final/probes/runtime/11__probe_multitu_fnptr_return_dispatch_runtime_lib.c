typedef int (*ProbeIntFn)(int);

static int inc3(int value) {
    return value + 3;
}

static int mul3(int value) {
    return value * 3;
}

ProbeIntFn probe_pick_transform(int which) {
    return which ? mul3 : inc3;
}
