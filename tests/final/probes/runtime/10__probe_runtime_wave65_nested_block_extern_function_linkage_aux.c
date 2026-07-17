int bucket10_wave65_nested_dispatch(int value);

int bucket10_wave65_nested_libb(int step) {
    {
        extern int bucket10_wave65_nested_dispatch(int value);
        int first = bucket10_wave65_nested_dispatch(step + 5);
        {
            int bucket10_wave65_nested_dispatch = step * 9;
            return first + bucket10_wave65_nested_dispatch;
        }
    }
}
