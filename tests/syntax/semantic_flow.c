int missing_return(int value) {
    if (value > 0) {
        return value;
    }
    value += 1;
}

int unreachable_block(void) {
    return 1;
    int after_return = 2;
    return after_return;
}

int switch_fallthrough(int x) {
    switch (x) {
        case 0:
            x = 1;
        case 1:
            x = 2;
            break;
        default:
            x = 3;
            break;
    }
    return x;
}
