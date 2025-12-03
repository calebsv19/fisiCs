#define CALL(fn, ...) fn(__VA_ARGS__)
#define ADD3(a, b, c) ((a) + (b) + (c))

int main(void) {
    // Variadic macro should forward all arguments into the callee.
    return CALL(ADD3, 1, 2, 3);
}
