#include <stdio.h>

struct wave354_const_payload {
    int left;
    int right;
};

extern const struct wave354_const_payload *const wave354_const_exported;
const struct wave354_const_payload *wave354_const_owner_get(void);

int main(void) {
    const struct wave354_const_payload *saved = wave354_const_exported;
    int stable = saved == wave354_const_owner_get();

    printf("%d %d %d\n", stable, saved->left, saved->right);
    return stable && saved->left == 31 && saved->right == 47 ? 0 : 1;
}
