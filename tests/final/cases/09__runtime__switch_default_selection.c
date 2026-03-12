int main(void) {
    int x = 0;
    int v = 5;
    switch (v) {
        default:
            x = 7;
            break;
        case 5:
            x = 9;
            break;
    }
    return (x == 9) ? 0 : 1;
}
