int add_one(int x) {
    return x + 1;
}

int (*fn_ptr)(int) = add_one;
