static int s_partition_hidden = 10;
int g_partition_visible = 3;

int abi_partition_step(int delta) {
    s_partition_hidden += delta * 2;
    g_partition_visible += delta;
    return s_partition_hidden * 7 + g_partition_visible;
}

int abi_partition_snapshot(void) {
    return s_partition_hidden * 7 + g_partition_visible;
}

int abi_partition_visible(void) {
    return g_partition_visible;
}
