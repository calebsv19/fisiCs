void sink(void) {}

int main(void) {
    int value = sink() ? 1 : 2;
    return value;
}
