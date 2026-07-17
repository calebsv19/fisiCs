#include <stdio.h>

struct wave354_payload {
    int left;
    int right;
};

extern struct wave354_payload *wave354_exported;
struct wave354_payload *wave354_owner_get(void);
int wave354_owner_shift(int delta);
int wave354_observer_alias_mutate(void);

int main(void) {
    struct wave354_payload *saved = wave354_exported;
    int initial_identity = saved == wave354_owner_get();
    int observer_ok = wave354_observer_alias_mutate();
    int owner_ok = wave354_owner_shift(3);
    int final_identity = saved == wave354_exported &&
                         saved == wave354_owner_get();

    printf("%d %d %d %d %d %d\n",
           initial_identity,
           observer_ok,
           owner_ok,
           final_identity,
           saved->left,
           saved->right);
    return initial_identity && observer_ok && owner_ok && final_identity &&
                   saved->left == 24 && saved->right == 41
               ? 0
               : 1;
}
