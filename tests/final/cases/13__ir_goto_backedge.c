int main(void) {
    int i = 0;
    int acc = 0;

start:
    if (i >= 5) {
        return acc;
    }

    acc += i * 3;
    ++i;
    goto start;
}
