#include <stdio.h>

static int wave80_visit(int n) {
    static int recurse_gate = 1;
    static int total = 0;
    total += n;
    if (n > 0 && recurse_gate) {
        recurse_gate = 0;
        total += wave80_visit(n - 1);
    }
    return total;
}

int main(void) {
    int first = wave80_visit(3);
    int second = wave80_visit(2);
    printf("%d %d\n", first, second);
    if (first != 10 || second != 12) {
        return 1;
    }
    return 0;
}
