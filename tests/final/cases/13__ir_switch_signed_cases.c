int main(void) {
    int x = -3;
    int out = 0;

    switch (x) {
        case -5:
            out = 50;
            break;
        case -3:
            out = 30;
            break;
        case 2:
            out = 2;
            break;
        default:
            out = 9;
            break;
    }

    return out;
}
