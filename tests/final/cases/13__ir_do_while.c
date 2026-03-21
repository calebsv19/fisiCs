int main(void) {
    int i = 0;
    int acc = 0;

    do {
        acc += i;
        i++;
    } while (i < 4);

    return acc;
}
