#include <stdio.h>

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

wave49_cell_fn wave49_callback_route_bridge(wave49_route_cb chooser, int selector);

static struct wave49_result wave49_local_a(struct wave49_cell item, int bias) {
    struct wave49_result out;
    out.left = item.x + bias;
    out.right = item.y - bias;
    out.sum = out.left * 3 + out.right * 5;
    return out;
}

static struct wave49_result wave49_local_b(struct wave49_cell item, int bias) {
    struct wave49_result out;
    out.left = item.x * bias;
    out.right = item.y + bias;
    out.sum = out.left + out.right * 7;
    return out;
}

static wave49_cell_fn wave49_choose_local(int selector) {
    if ((selector & 1) != 0) {
        return wave49_local_b;
    }
    return wave49_local_a;
}

int main(void) {
    struct wave49_cell item = {6, 11};
    wave49_cell_fn selected = wave49_callback_route_bridge(wave49_choose_local, 3);
    struct wave49_result got = selected(item, 4);
    printf("%d %d %ld\n", got.left, got.right, got.sum);
    return 0;
}
