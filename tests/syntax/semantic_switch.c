int f(int v) {
    switch (v) {
        case 1:
            return 1;
        case 1: // duplicate
            return 2;
        default:
            break;
        default: // duplicate default
            return 3;
    }
    return 0;
}
