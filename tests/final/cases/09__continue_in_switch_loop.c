int main(void) {
    int i;
    int sum = 0;
    for (i = 0; i < 4; ++i) {
        switch (i) {
            case 1:
                continue;
            default:
                sum += 1;
                break;
        }
    }
    return sum == 3 ? 0 : 1;
}
