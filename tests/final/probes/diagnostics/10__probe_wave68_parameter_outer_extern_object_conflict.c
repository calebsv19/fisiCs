int wave68_shared_object = 41;

int wave68_outer_extern_object(int wave68_shared_object) {
    extern int wave68_shared_object;
    return wave68_shared_object;
}

int main(void) {
    return wave68_outer_extern_object(7) == 41 ? 0 : 1;
}
