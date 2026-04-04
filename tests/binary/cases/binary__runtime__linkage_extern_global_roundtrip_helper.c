int linkage_counter = 10;

void linkage_add(int delta) {
    linkage_counter += delta;
}

int linkage_get(void) {
    return linkage_counter;
}
