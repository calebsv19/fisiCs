#include <stdio.h>

typedef struct SpeedState {
    float displayed_sim_speed;
} SpeedState;

static void render_speed(char *out, size_t out_cap, const SpeedState *state) {
    snprintf(out, out_cap, "%.2fx", state->displayed_sim_speed);
}

int main(void) {
    SpeedState state;
    char text[32];

    state.displayed_sim_speed = 1.25f;
    render_speed(text, sizeof(text), &state);
    printf("%s\n", text);
    printf("%.2f\n", state.displayed_sim_speed);
    return 0;
}
