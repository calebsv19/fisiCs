struct Wave40DirectBox {
    int value;
};

int wave40_direct_hard(void) {
    return (int)(struct Wave40DirectBox){ 1 };
}
