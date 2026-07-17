int bucket10_wave64_nested_current_external(int value) {
    static int state = 40;

    state += value;
    return state + value;
}
