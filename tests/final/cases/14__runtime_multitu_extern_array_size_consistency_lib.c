int g_size_bridge_table[6] = {3, 5, 8, 13, 21, 34};

int abi_size_bridge_len(void) {
    return (int)(sizeof(g_size_bridge_table) / sizeof(g_size_bridge_table[0]));
}

int abi_size_bridge_get(int idx) {
    return g_size_bridge_table[idx];
}

int abi_size_bridge_weighted_sum(void) {
    int sum = 0;
    for (int i = 0; i < 6; ++i) {
        sum += g_size_bridge_table[i] * (i + 1);
    }
    return sum;
}
