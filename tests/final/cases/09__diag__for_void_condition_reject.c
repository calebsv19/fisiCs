void sink(void) {}

int main(void) {
    for (; sink();) {
        return 0;
    }
    return 1;
}
