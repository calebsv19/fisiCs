struct Pair {
    int value;
};

int main(void) {
    int flag = 1;
    return flag ? (struct Pair){1} : (struct Pair){2};
}
