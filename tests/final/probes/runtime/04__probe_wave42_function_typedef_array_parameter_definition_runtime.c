extern int printf(const char *, ...);

typedef int Wave42Row[3];
typedef int Wave42RowConsumer(Wave42Row);

Wave42RowConsumer wave42_consume_row;

int wave42_consume_row(int *values) {
    return values[0] * 10 + values[2];
}

int main(void) {
    Wave42Row values = {2, 4, 7};
    printf("%d\n", wave42_consume_row(values));
    return 0;
}
