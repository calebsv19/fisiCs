int main(void) {
    int i = 0;
    int acc = 0;

    while (i < 5) {
        if (i == 2) {
            ++i;
            goto after_add;
        }
        acc += i;
after_add:
        ++i;
    }

    return acc;
}
