struct wave354_const_payload {
    int left;
    int right;
};

const struct wave354_const_payload *const wave354_const_exported =
    &(const struct wave354_const_payload){31, 47};

const struct wave354_const_payload *wave354_const_owner_get(void) {
    return wave354_const_exported;
}
