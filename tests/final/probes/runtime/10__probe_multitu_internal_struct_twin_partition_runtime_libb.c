struct Bucket10TwinStruct {
    int left;
    int right;
};

static struct Bucket10TwinStruct bucket10_private_struct = {7, 3};

int bucket10_struct_twin_step_b(int delta_left, int delta_right) {
    bucket10_private_struct.left += delta_left;
    bucket10_private_struct.right += delta_right;
    return bucket10_private_struct.left + bucket10_private_struct.right;
}

int bucket10_struct_twin_peek_b(void) {
    return bucket10_private_struct.left + bucket10_private_struct.right;
}
