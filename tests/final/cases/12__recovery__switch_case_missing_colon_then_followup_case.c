int main(void) {
    int x = 2;
    switch (x) {
        case 0
            x = 10;
            break;
        case 1:
            x = x + 2;
            break;
        default:
            x = x + 3;
            break;
    }
    return x;
}
