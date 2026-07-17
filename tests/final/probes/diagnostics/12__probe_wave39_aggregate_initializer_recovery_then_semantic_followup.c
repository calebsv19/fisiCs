#line 13440 "virtual_wave39_aggregate_initializer_recovery.c"
struct Pair {
    int left;
    int right;
};

int main(void) {
    struct Pair pair = { 1 2 };
    int later = wave39_initializer_missing;
    int *ptr = later;
    return pair.left + later + *ptr;
}
