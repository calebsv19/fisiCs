#include <stdio.h>

static int wave27_rebind_pointer_to_const(const int selected[4],
                                          const int replacement[4]) {
    int first = selected[0];
    selected = replacement;
    return first * 10 + selected[3];
}

int main(void) {
    const int first[4] = {2, 0, 0, 0};
    const int second[4] = {0, 0, 0, 7};
    printf("%d\n", wave27_rebind_pointer_to_const(first, second));
    return 0;
}
