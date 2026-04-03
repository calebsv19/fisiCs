typedef struct {
    int a;
    int b;
} AbiState;

extern AbiState abi_state_global;

int abi_state_sum(void) {
    return abi_state_global.a + abi_state_global.b;
}
