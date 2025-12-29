int annotated(__attribute__((unused)) int x, int y [[maybe_unused]]);

int main(void) {
    __attribute__((cold)) if (0) return 1;
    [[maybe_unused]] while (0) { break; }
    __attribute__((unused)) return 0;
}
