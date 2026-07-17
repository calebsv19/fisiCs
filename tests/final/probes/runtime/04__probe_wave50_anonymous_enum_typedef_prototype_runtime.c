typedef enum {
    WAVE50_MODE_IDLE = 0,
    WAVE50_MODE_EDIT = 3
} Wave50Mode;

typedef struct {
    int value;
} Wave50Context;

void wave50_apply_mode(Wave50Context* context, Wave50Mode mode, int delta);
const char* wave50_mode_label(Wave50Mode mode);

void wave50_apply_mode(Wave50Context* context, Wave50Mode mode, int delta) {
    context->value = (int)mode + delta;
}

const char* wave50_mode_label(Wave50Mode mode) {
    return mode == WAVE50_MODE_EDIT ? "edit" : "idle";
}

int main(void) {
    Wave50Context context = {0};
    wave50_apply_mode(&context, WAVE50_MODE_EDIT, 4);
    return context.value == 7 && wave50_mode_label(WAVE50_MODE_EDIT)[0] == 'e'
               ? 0
               : 1;
}
