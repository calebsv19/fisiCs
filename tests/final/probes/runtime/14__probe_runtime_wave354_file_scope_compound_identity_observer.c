struct wave354_payload {
    int left;
    int right;
};

extern struct wave354_payload *wave354_exported;
struct wave354_payload *wave354_owner_get(void);

int wave354_observer_alias_mutate(void) {
    struct wave354_payload *direct = wave354_exported;
    struct wave354_payload *through_owner = wave354_owner_get();

    if (direct != through_owner) {
        return 0;
    }

    direct->left += 4;
    through_owner->right += 6;
    return direct == wave354_exported &&
           wave354_exported->left == 21 &&
           wave354_exported->right == 35;
}
