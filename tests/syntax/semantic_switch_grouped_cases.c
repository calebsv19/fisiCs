int grouped_cases_no_fallthrough_warning(int x) {
    switch (x) {
        case 0:
        case 1:
            x = 10;
            break;
        default:
            x = 20;
            break;
    }
    return x;
}
