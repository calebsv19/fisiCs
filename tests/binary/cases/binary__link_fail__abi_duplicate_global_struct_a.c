typedef struct {
    int a;
    int b;
} AbiState;

AbiState abi_dup_state = {1, 2};

int main(void) {
    return abi_dup_state.a + abi_dup_state.b;
}
