int main(void) {
    int sum = 0;
    for (int i = 0; i < 5; ++i) {
        switch (i) {
            case 1:
                continue;
            case 3:
                break;
            default:
                sum += i;
                break;
        }
    }
    return (sum == 6) ? 0 : 1;
}
