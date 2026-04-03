typedef struct {
    int a;
    int b;
} Pair;

static int sum_val(Pair p) {
    return p.a + p.b;
}

int main(void) {
    Pair v = {1, 2};
    Pair *pp = &v;
    return sum_val(pp);
}
