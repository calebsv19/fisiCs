#include <stdio.h>

typedef int Wave27Row[4];

static int wave27_rebind_row(Wave27Row selected, Wave27Row replacement) {
    int first = selected[0];
    selected = replacement;
    return first * 10 + selected[3];
}

int main(void) {
    Wave27Row first = {2, 0, 0, 0};
    Wave27Row second = {0, 0, 0, 7};
    printf("%d\n", wave27_rebind_row(first, second));
    return 0;
}
