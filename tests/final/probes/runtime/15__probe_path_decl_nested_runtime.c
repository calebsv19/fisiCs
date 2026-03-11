typedef int (*fn_t)(int);

fn_t choose(fn_t a, fn_t b, int flag) {
    return flag ? a : b;
}

int add1(int x) {
    return x + 1;
}

int sub1(int x) {
    return x - 1;
}

int main(void) {
    fn_t f = choose(add1, sub1, 1);
    return f(9) == 10 ? 0 : 1;
}
