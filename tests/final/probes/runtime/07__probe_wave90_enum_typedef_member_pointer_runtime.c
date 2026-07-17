typedef enum {
    WAVE90_IDLE = 0,
    WAVE90_ACTIVE = 5
} Wave90Mode;

typedef struct {
    Wave90Mode mode;
} Wave90State;

static void wave90_set_mode(Wave90Mode* out_mode, Wave90Mode mode) {
    *out_mode = mode;
}

int main(void) {
    Wave90State state = { WAVE90_IDLE };
    wave90_set_mode(&state.mode, WAVE90_ACTIVE);
    return state.mode == WAVE90_ACTIVE ? 0 : 1;
}
