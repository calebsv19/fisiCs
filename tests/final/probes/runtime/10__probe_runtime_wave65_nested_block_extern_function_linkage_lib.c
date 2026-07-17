int bucket10_wave65_nested_dispatch(int value) {
    static int state = 70;

    state += value;
    return state + value;
}

int bucket10_wave65_nested_liba(int step) {
    extern int bucket10_wave65_nested_dispatch(int value);
    int first = bucket10_wave65_nested_dispatch(step);

    {
        int bucket10_wave65_nested_dispatch = first + step;
        return bucket10_wave65_nested_dispatch + first;
    }
}
