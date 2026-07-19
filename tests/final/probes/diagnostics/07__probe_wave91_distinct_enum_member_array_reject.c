typedef enum { LEFT_A, LEFT_B } LeftButton;
typedef enum { RIGHT_A, RIGHT_B } RightButton;

typedef struct {
    LeftButton order[4];
} Options;

static void update_order(RightButton order[4]) {
    order[0] = RIGHT_B;
}

void apply(Options *options) {
    update_order(options->order);
}
