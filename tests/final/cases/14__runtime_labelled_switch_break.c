#include <stdio.h>

int main(void) {
    int value = 2;
    int acc = 0;

    switch (value) {
        case 2:
            acc += 3;
labelled_case:
            acc += 4;
            break;
        default:
            acc = -1;
            break;
    }

    if (acc == 7) {
        goto labelled_exit;
    }
    acc = -2;

labelled_exit:
    printf("%d\n", acc);
    return 0;
}
