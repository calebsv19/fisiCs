int main(void) {
    int total = 0;

    for (int i = 0; i < 4; ++i) {
        switch (i) {
            case 1:
                total += 100;
                break;
            case 2:
                total += 20;
                break;
            default:
                total += 1;
                break;
        }

        total += 3;
    }

    return (total == 134) ? 0 : 1;
}
