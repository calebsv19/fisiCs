int dispatch(int seed) {
    int total = 0;
    for (int i = 0; i < 3; ++i) {
        switch ((seed + i) % 4) {
            case 0:
                total += 1;
                break;
            case 1:
                total += 2;
                if (i == 1) {
                    break;
                }
                /* fallthrough to default */
            case 3:
                total += 5;
                break;
            default:
                total += 7;
                if (total > 10) {
                    continue;
                }
                switch (i) {
                    case 0:
                        total += 11;
                        break;
                    default:
                        total += 13;
                        break;
                }
                break;
        }
        total += 100;
    }
    return total;
}

int main(void) {
    return dispatch(2);
}
