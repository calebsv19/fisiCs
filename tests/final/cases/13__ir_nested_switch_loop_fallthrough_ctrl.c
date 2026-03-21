int main(void) {
    int acc = 0;

    for (int i = 0; i < 8; ++i) {
        switch (i % 4) {
            case 0:
                acc += 1;
            case 1:
                acc += 10;
                continue;
            case 2:
                acc += 100;
                break;
            default:
                acc += 1000;
                break;
        }

        if (i == 6) {
            break;
        }
        acc += i;
    }

    return acc;
}
