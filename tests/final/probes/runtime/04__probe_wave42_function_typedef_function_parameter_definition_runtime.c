extern int printf(const char *, ...);

typedef int Wave42Unary(int);
typedef Wave42Unary Wave42UnaryAlias;
typedef int Wave42Apply(Wave42UnaryAlias, int);

Wave42Apply wave42_apply;

static int wave42_twice(int value) {
    return value * 2;
}

int wave42_apply(int (*callback)(int), int value) {
    return callback(value) + 3;
}

int main(void) {
    printf("%d\n", wave42_apply(wave42_twice, 6));
    return 0;
}
