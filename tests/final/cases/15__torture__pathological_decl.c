typedef int (*unary_fn_t)(int);

int increment(int x) {
    return x + 1;
}

unary_fn_t take_and_return(unary_fn_t fn) {
    return fn;
}

int main(void) {
    unary_fn_t fn = take_and_return(increment);
    return fn(4) == 5 ? 0 : 1;
}
