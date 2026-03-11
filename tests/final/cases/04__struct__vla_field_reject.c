int vla_field_reject_fn(int n) {
    struct VLAFieldReject {
        int data[n];
    };
    return 0;
}
