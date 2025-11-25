__attribute__((aligned(16)))
int global_value;

struct __attribute__((packed)) Example {
    __declspec(dllexport) int exported;
};

[[gnu::cold]]
int flagged(void) {
    return global_value;
}
