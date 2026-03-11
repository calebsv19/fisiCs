int main(void) {
    int v = 2;
    switch (v) {
        case 1 ... 3:
            return 0;
        default:
            return 1;
    }
}
