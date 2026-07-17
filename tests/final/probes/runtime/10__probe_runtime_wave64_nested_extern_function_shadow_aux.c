int bucket10_wave64_nested_liba(int step) {
    extern int bucket10_wave64_nested_external(int);
    int first = bucket10_wave64_nested_external(step);
    {
        int bucket10_wave64_nested_external = first + step;
        return bucket10_wave64_nested_external + first;
    }
}

int bucket10_wave64_nested_libb(int step) {
    extern int bucket10_wave64_nested_external(int);
    int first = bucket10_wave64_nested_external(step);
    {
        int bucket10_wave64_nested_external = first + step;
        return bucket10_wave64_nested_external + first;
    }
}
