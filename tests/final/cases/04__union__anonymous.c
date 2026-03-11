union UnionWithAnonymous {
    int tag;
    union {
        int i;
        float f;
    };
};
