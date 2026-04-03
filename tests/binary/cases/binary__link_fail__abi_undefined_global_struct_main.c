typedef struct {
    int a;
    int b;
} AbiState;

int abi_state_sum(void);

int main(void) {
    return abi_state_sum();
}
