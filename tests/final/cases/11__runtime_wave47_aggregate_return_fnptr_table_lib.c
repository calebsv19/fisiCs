typedef struct Segment47A {
    int left;
    int right;
} Segment47A;

typedef struct Bundle47A {
    Segment47A slots[2];
    int bias;
} Bundle47A;

typedef Bundle47A (*BundleMaker47A)(Segment47A, int);

static Bundle47A make_add47(Segment47A seed, int scale) {
    Bundle47A bundle;
    bundle.slots[0].left = seed.left + scale;
    bundle.slots[0].right = seed.right + scale;
    bundle.slots[1].left = seed.left + scale + 10;
    bundle.slots[1].right = seed.right + scale + 20;
    bundle.bias = scale + 2;
    return bundle;
}

static Bundle47A make_mul47(Segment47A seed, int scale) {
    Bundle47A bundle;
    bundle.slots[0].left = seed.left * scale;
    bundle.slots[0].right = seed.right * scale;
    bundle.slots[1].left = seed.left + scale + 10;
    bundle.slots[1].right = seed.right + scale + 20;
    bundle.bias = scale + 5;
    return bundle;
}

BundleMaker47A wave47_select_bundle_maker(int route) {
    static BundleMaker47A table[2] = {make_add47, make_mul47};
    return table[route & 1];
}
