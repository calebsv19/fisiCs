typedef enum { BUTTON_A, BUTTON_B } Button;

typedef struct {
    Button order[4];
} Options;

static void update_order(Button order[4]) {
    order[0] = BUTTON_B;
}

int main(void) {
    Options options = {{BUTTON_A, BUTTON_A, BUTTON_A, BUTTON_A}};
    update_order(options.order);
    return options.order[0] == BUTTON_B ? 0 : 1;
}
