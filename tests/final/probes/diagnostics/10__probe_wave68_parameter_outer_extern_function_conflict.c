int wave68_route(void) {
    return 37;
}

int wave68_outer_extern_function(int wave68_route) {
    extern int wave68_route(void);
    return wave68_route();
}

int main(void) {
    return wave68_outer_extern_function(7) == 37 ? 0 : 1;
}
