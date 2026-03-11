int main(void) {
    int i = 0;
    int sum = 0;

    for (;;) {
        break;
    }

    for (i = 0; i < 2;) {
        sum += i;
        i++;
    }

    for (; i < 4; i++) {
        sum += i;
    }

    return sum;
}
