int source_value(void) {
    return 7;
}

int use_static_local(void) {
    int seed = source_value();
    static int cache = seed;
    return cache;
}

int main(void) {
    return use_static_local();
}
