struct Bucket10ArrayNode {
    int left;
    int right;
};

static struct Bucket10ArrayNode lane[3] = {
    {2, 1},
    {4, 3},
    {6, 5},
};

int bucket10_local_struct_array_step_a(int index, int delta) {
    lane[index].left += delta;
    lane[index].right += delta + index;
    return lane[0].left + lane[1].left + lane[2].right;
}

int bucket10_local_struct_array_peek_a(void) {
    return lane[0].left + lane[0].right +
           lane[1].left + lane[1].right +
           lane[2].left + lane[2].right;
}
