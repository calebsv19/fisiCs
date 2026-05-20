int main(void) {
    int i = 0;
    int sum = 0;

    do {
        switch (i) {
            case 0:
                sum += 1;
                break;
            case 1:
                i = i + 1;
                continue;
            default:
                sum += 5;
                break;
        }

        sum += 10;
        i = i + 1;
    } while (i < 3);

    return (sum == 26 && i == 3) ? 0 : 1;
}
