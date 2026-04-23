#define BAD_JOINV(a, ...) a ## __VA_ARGS__

int main(void) {
    return BAD_JOINV(1, +);
}
