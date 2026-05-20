typedef int (*ProbeIntFnInc)(int);

static int inc5(int value) {
    return value + 5;
}

static int mul4(int value) {
    return value * 4;
}

ProbeIntFnInc probe_pick_transform_inc(int which) {
    return which ? mul4 : inc5;
}
