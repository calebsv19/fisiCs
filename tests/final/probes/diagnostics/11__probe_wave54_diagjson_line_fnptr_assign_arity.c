#line 17001 "virtual_wave54_fnptr_assign_arity.c"
int wave54_one(int value) { return value; }
int wave54_two(int left, int right) { return left + right; }

int main(void) {
    int (*target)(int) = wave54_one;
    int (*source)(int, int) = wave54_two;
    target = source;
    return target(1);
}
