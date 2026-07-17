extern int printf(const char*, ...);

struct Pair {
    int left;
    int right;
};

int main(void) {
    struct Pair items[2] = {{4, 9}, {6, 11}};
    int trace = 1;
    int choose = 1;
    int n = 3;

    int width = (int)sizeof(int[(int)(choose ? (trace += 2, n + 1) : (trace += 100, n + 7))]);
    int selected = *(int *)(void *)(choose ? &items[0].right : &items[1].left);
    int value = width + selected;

    printf("%d %d %d %d\n", trace, width, selected, value);
    return 0;
}
