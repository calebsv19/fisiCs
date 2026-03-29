typedef int (*LaneOp)(int, int);

static int lane_add(int x, int y) {
    return x + y * 2 + 3;
}

static int lane_mix(int x, int y) {
    return (x ^ (y * 9 + 5)) + (x & 7);
}

static int lane_sub(int x, int y) {
    return x - y * 4 + 19;
}

LaneOp select_lane(int idx) {
    int norm = idx % 3;
    if (norm < 0) {
        norm += 3;
    }

    if (norm == 0) {
        return lane_add;
    }
    if (norm == 1) {
        return lane_mix;
    }
    return lane_sub;
}

int reduce_state(int state, int salt) {
    int out = (state * 7 + salt * 5 + 3) % 100003;
    if (out < 0) {
        out += 100003;
    }
    return out;
}
