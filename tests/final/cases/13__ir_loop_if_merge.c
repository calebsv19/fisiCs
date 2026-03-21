int main(void) {
    int i = 0;
    int acc = 0;

    while (i < 6) {
        if ((i & 1) == 0) {
            acc += i;
        } else {
            acc += i * 2;
        }
        ++i;
    }

    return acc;
}
