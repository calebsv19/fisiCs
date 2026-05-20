int main(void) {
    int sum = 0;

    switch (2) {
        case 2:
            for (int i = 0; i < 4; ++i) {
                if (i == 1) {
                    continue;
                }
                sum += i;
            }
            break;
        default:
            return 1;
    }

    return (sum == 5) ? 0 : 1;
}
