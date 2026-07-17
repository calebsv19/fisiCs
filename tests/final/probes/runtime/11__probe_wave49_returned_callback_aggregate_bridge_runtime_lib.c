struct wave49_cell {
    int x;
    int y;
};

struct wave49_result {
    int left;
    int right;
    long sum;
};

typedef struct wave49_result (*wave49_cell_fn)(struct wave49_cell item, int bias);
typedef wave49_cell_fn (*wave49_route_cb)(int selector);

static struct wave49_result wave49_lib_fallback(struct wave49_cell item, int bias) {
    struct wave49_result out;
    out.left = item.x - bias;
    out.right = item.y + bias * 2;
    out.sum = out.left * 11 + out.right;
    return out;
}

wave49_cell_fn wave49_callback_route_bridge(wave49_route_cb chooser, int selector) {
    wave49_cell_fn picked = chooser(selector);
    if (selector < 0) {
        return wave49_lib_fallback;
    }
    return picked;
}
