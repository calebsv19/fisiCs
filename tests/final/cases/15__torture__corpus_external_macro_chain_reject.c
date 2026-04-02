#define CHAIN_STEP(x) x
#define CHAIN_JOIN(a, b) a ## b
#define CHAIN_CALL(x

int main(void) {
    return CHAIN_JOIN(CHAIN_STEP(2), 1);
}
