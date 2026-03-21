int main(void) {
    int x = 0;
    int y = 2;
    int out = 0;

    switch (x) {
        case 0:
            switch (y) {
                case 1:
                    out = 11;
                    break;
                case 2:
                    out = 22;
                    break;
                default:
                    out = 33;
                    break;
            }
            break;
        default:
            out = 44;
            break;
    }

    return out;
}
