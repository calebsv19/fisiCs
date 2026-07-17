struct wave70_payload {
    int left;
    int right;
};

struct wave70_payload *wave70_external_first =
    &(struct wave70_payload){17, 23};
struct wave70_payload *wave70_external_second =
    &(struct wave70_payload){17, 23};

static struct wave70_payload *wave70_local =
    &(struct wave70_payload){17, 23};

struct wave70_payload *wave70_owner_local(void) {
    return wave70_local;
}

int wave70_owner_mutate(void) {
    wave70_external_first->left += 1;
    wave70_external_second->right += 2;
    wave70_local->left += 3;
    wave70_local->right += 3;
    return 1;
}
