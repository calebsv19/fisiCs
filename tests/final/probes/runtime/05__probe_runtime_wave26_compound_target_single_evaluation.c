extern int printf(const char*, ...);

struct Pair {
    int left;
    int right;
};

static int picks;

static int *select_member(struct Pair *pair, int right) {
    picks += 1;
    return right ? &pair->right : &pair->left;
}

int main(void) {
    struct Pair pair = {8, 13};

    *select_member(&pair, 1) += 5;
    *select_member(&pair, 0) *= 3;
    *select_member(&pair, pair.left > pair.right) -= 2;

    printf("%d %d %d\n", picks, pair.left, pair.right);
    return 0;
}
