static int g_static = 7;

int read_static(void) {
    static int local_counter = 0;
    local_counter = local_counter + 1;
    return g_static + local_counter;
}
