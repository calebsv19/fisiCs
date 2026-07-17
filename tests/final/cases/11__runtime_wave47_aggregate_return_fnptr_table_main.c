#include <stdio.h>

typedef struct Segment47A {
    int left;
    int right;
} Segment47A;

typedef struct Bundle47A {
    Segment47A slots[2];
    int bias;
} Bundle47A;

typedef Bundle47A (*BundleMaker47A)(Segment47A, int);

BundleMaker47A wave47_select_bundle_maker(int route);

int main(void) {
    Segment47A seed = {5, 7};
    BundleMaker47A maker = wave47_select_bundle_maker(1);
    Bundle47A bundle = maker(seed, 3);
    int checksum = bundle.slots[0].left + bundle.slots[0].right +
        bundle.slots[1].left + bundle.slots[1].right + bundle.bias;
    if (bundle.slots[0].left != 15) return 1;
    if (bundle.slots[1].right != 30) return 2;
    if (checksum != 92) return 3;
    printf("%d %d %d\n", checksum, bundle.slots[1].right, bundle.bias);
    return 0;
}
