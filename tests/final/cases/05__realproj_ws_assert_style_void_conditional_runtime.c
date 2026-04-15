static void set_fail(int* state) {
    *state = -1;
}

static void assert_style_path(int condition, int* state) {
    (!condition) ? set_fail(state) : (void)0;
}

int main(void) {
    int state = 0;
    assert_style_path(1, &state);
    if (state != 0) {
        return 1;
    }

    assert_style_path(0, &state);
    if (state != -1) {
        return 2;
    }

    return 0;
}
