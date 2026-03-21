int main(void) {
    int sum = 0;

    for (int i = 0; i < 6; ++i) {
        switch (i & 1) {
            case 0:
                sum += 1;
                continue;
            default:
                sum += 10;
                break;
        }

        sum += 100;
    }

    return sum;
}
