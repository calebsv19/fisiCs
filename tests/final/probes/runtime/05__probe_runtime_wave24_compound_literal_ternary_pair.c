extern int printf(const char*, ...);

struct Pair {
    int left;
    int right;
};

int main(void) {
    int choose = 0;
    int trace = 2;

    struct Pair picked = choose ? (struct Pair){(trace += 3), 11} : (struct Pair){(trace += 5), 17};
    struct Pair mirrored = choose ? (struct Pair){picked.left + 1, picked.right + 1} : (struct Pair){picked.right, picked.left};
    int total = picked.left + mirrored.left;

    printf("%d %d %d %d %d\n", trace, picked.left, picked.right, mirrored.left, total);
    return 0;
}
