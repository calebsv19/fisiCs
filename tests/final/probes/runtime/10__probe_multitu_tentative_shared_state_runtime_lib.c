extern int shared_count;

int bump_shared(void) {
    shared_count += 5;
    return shared_count;
}
