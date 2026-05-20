struct Bucket10TwinArrayNode {
    int left;
    int right;
};

static struct Bucket10TwinArrayNode bucket10_private_lane[2] = {
    {5, 1},
    {2, 6},
};

int bucket10_struct_array_twin_step_b(int index, int delta) {
    bucket10_private_lane[index].left += delta;
    bucket10_private_lane[index].right -= delta - index;
    return bucket10_private_lane[0].left + bucket10_private_lane[0].right +
           bucket10_private_lane[1].left + bucket10_private_lane[1].right;
}

int bucket10_struct_array_twin_peek_b(void) {
    return bucket10_private_lane[0].left + bucket10_private_lane[0].right +
           bucket10_private_lane[1].left + bucket10_private_lane[1].right;
}
