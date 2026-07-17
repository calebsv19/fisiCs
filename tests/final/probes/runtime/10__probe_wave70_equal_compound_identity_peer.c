struct wave70_payload {
    int left;
    int right;
};

static struct wave70_payload *wave70_local =
    &(struct wave70_payload){17, 23};

struct wave70_payload *wave70_peer_local(void) {
    return wave70_local;
}

int wave70_peer_mutate(void) {
    wave70_local->left += 4;
    wave70_local->right += 8;
    return 1;
}
