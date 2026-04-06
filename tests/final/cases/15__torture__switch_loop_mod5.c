#include <stdio.h>

int main(void) {
    int acc = 0;
    int i;
    for (i = 0; i < 20; ++i) {
        switch (i % 5) {
            case 0:
                acc += 1;
                break;
            case 1:
                acc += 2;
                break;
            case 2:
                acc += 3;
                break;
            case 3:
                acc += 4;
                break;
            default:
                acc += 5;
                break;
        }
    }
    printf("%d\n", acc);
    return 0;
}
