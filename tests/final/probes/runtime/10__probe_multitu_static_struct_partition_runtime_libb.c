struct Bucket10LocalStruct {
    int left;
    int right;
};

static struct Bucket10LocalStruct lane = {6, 2};

int bucket10_local_struct_step_b(int delta_left, int delta_right) {
    lane.left += delta_left;
    lane.right += delta_right;
    return lane.left + lane.right;
}

int bucket10_local_struct_peek_b(void) {
    return lane.left + lane.right;
}
