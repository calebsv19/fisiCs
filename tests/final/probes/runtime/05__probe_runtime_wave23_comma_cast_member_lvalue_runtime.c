extern int printf(const char*, ...);

struct Pair {
    int left;
    int right;
};

int main(void) {
    struct Pair a = {3, 8};
    struct Pair b = {5, 13};
    struct Pair *items[2] = {&a, &b};
    int trace = 0;
    int pick = 1;

    int *slot = (int *)(void *)&((trace += 2, items[pick])->right);
    *slot += (int)sizeof(((trace += 80), items[0]->left)) + (trace += 3, ((struct Pair *)(void *)items[0])->right);

    pick = 0;
    int *slot2 = (int *)(void *)&((trace += 5, items[pick])->left);
    *slot2 += (trace += 7, *slot);

    printf("%d %d %d %d %d\n", trace, a.left, a.right, b.left, b.right);
    return 0;
}
