extern int printf(const char*, ...);

struct Pair {
    int x;
    int y;
};

static struct Pair make_pair(int *trace, int delta) {
    *trace += delta;
    struct Pair pair = {*trace, *trace + 1};
    return pair;
}

int main(void) {
    int trace = 5;
    int choose = 0;
    int n = 2;

    int vla_bytes = (int)sizeof(int[choose ? (trace += 50, n + 1) : (trace += 3, n + 2)]);
    struct Pair picked = choose ? make_pair(&trace, 10) : make_pair(&trace, 20);
    int nonvla_bytes = (int)sizeof(choose ? make_pair(&trace, 100) : picked);

    printf("%d %d %d %d %d\n", trace, vla_bytes, nonvla_bytes, picked.x, picked.y);
    return 0;
}
