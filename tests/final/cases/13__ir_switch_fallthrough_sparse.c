int main(void) {
    int x = 2;
    int out = 0;

    switch (x) {
        case 0:
            out += 1;
        case 1:
            out += 10;
            break;
        case 2:
            out += 100;
        default:
            out += 1000;
            break;
    }

    return out;
}
