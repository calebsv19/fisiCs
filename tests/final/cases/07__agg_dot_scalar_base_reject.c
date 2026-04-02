struct Wrap {
    int value;
};

int main(void) {
    struct Wrap w = {3};
    return w.value.missing;
}
