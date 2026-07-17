typedef int Wave42Unary(int);
typedef int Wave42Apply(Wave42Unary, int);

Wave42Apply wave42_apply_conflict;

int wave42_apply_conflict(long (*callback)(int), int value) {
    return (int)callback(value);
}

int main(void) {
    return 0;
}
