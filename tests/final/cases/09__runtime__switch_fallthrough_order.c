int main(void) {
    int x = 0;
    switch (2) {
        case 1:
            x += 1;
        case 2:
            x += 4;
        case 3:
            x += 8;
            break;
        default:
            x = 99;
            break;
    }
    return (x == 12) ? 0 : 1;
}
