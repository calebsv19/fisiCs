int main(void) {
    int i = 0;
    int acc = 0;

    do {
        ++i;
        if (i == 2) {
            continue;
        }
        if (i == 5) {
            break;
        }
        acc += i;
    } while (i < 8);

    return acc;
}
