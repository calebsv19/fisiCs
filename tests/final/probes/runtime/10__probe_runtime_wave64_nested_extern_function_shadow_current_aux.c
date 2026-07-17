int bucket10_wave64_nested_current_external(int value);

int bucket10_wave64_nested_current_liba(int step) {
    int first = bucket10_wave64_nested_current_external(step);
    {
        int bucket10_wave64_nested_current_external = first + step;
        return bucket10_wave64_nested_current_external + first;
    }
}

int bucket10_wave64_nested_current_libb(int step) {
    int first = bucket10_wave64_nested_current_external(step);
    {
        int bucket10_wave64_nested_current_external = first + step;
        return bucket10_wave64_nested_current_external + first;
    }
}
