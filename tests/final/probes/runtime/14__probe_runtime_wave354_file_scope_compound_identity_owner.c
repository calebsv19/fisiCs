struct wave354_payload {
    int left;
    int right;
};

struct wave354_payload *wave354_exported =
    &(struct wave354_payload){17, 29};

struct wave354_payload *wave354_owner_get(void) {
    return wave354_exported;
}

int wave354_owner_shift(int delta) {
    struct wave354_payload *before = wave354_exported;
    wave354_exported->left += delta;
    wave354_exported->right += delta * 2;
    return before == wave354_exported;
}
