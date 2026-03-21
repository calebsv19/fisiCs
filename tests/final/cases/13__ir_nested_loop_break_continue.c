int main(void) {
    int acc = 0;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (((i + j) & 1) != 0) {
                continue;
            }
            if (i == 3) {
                break;
            }
            acc += i * 10 + j;
        }
        acc += 100;
    }

    return acc;
}
