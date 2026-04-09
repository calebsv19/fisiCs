#define CHOOSE(flag, a, b) ((flag) ? (a) : (b))

int main(void) {
    int enabled = 0;
    int value = CHOOSE(enabled, 1);
    return value;
}
