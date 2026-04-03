typedef struct {
    int a;
    int b;
} Pair;

Pair abi_make_pair(void);

int main(void) {
    Pair value = abi_make_pair();
    return value.a + value.b;
}
