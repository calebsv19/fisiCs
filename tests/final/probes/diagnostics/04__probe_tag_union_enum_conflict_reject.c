union DualTag {
    int value;
};

enum DualTag {
    DUAL_TAG_VALUE = 1
};

int main(void) {
    return DUAL_TAG_VALUE - 1;
}
