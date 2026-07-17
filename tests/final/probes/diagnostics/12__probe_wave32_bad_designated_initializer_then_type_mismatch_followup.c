struct wave32_pair {
    int left;
    int right;
};

int main(void) {
    struct wave32_pair pair = {.left = 1, . = 2, .right = 3};
    int *ptr = 0;
    int value = ptr;
    return pair.left + value;
}
