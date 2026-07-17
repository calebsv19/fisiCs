#line 17101 "virtual_wave54_fnptr_too_many_args.c"
int wave54_one(int value) { return value; }

int main(void) {
    int (*target)(int) = wave54_one;
    return target(1, 2);
}
