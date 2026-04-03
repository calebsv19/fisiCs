typedef struct {
    int a;
    int b;
} Pair;

static int sum_pair(Pair p) {
    return p.a + p.b;
}

int main(void) {
    return sum_pair(42);
}
