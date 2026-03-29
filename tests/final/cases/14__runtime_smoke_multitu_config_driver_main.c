#include <stdio.h>

struct Config {
    int base;
    int stride;
    int mode;
};

const struct Config *fetch_config(int idx);
int apply_config(const struct Config *cfg, int x);
int fold_config_trace(int acc, int i, int value);

int main(void) {
    int inputs[] = {3, 1, 4, 1, 5, 9};
    int state = 17;
    int trace = 0;
    int n = (int)(sizeof(inputs) / sizeof(inputs[0]));
    int i;

    for (i = 0; i < n; ++i) {
        const struct Config *cfg = fetch_config(i + (state % 3));
        int value = apply_config(cfg, inputs[i] + (state & 1));
        state = (state * 5 + value + i) % 10007;
        trace = fold_config_trace(trace, i, value + cfg->mode);
    }

    printf("%d %d\n", state, trace);
    return 0;
}
