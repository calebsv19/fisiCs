typedef enum { WAVE90_LEFT = 1 } Wave90Left;
typedef enum { WAVE90_RIGHT = 2 } Wave90Right;

typedef struct {
    Wave90Left value;
} Wave90Box;

static void wave90_take_right(Wave90Right* value) {
    (void)value;
}

int main(void) {
    Wave90Box box = { WAVE90_LEFT };
    wave90_take_right(&box.value);
    return 0;
}
