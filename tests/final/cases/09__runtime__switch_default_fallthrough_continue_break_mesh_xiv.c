#include <stdio.h>

int main(void) {
    int selectors[] = {5, 2, 4, 7, 2, 9};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        switch (selectors[i]) {
            default:
                acc += 1;
            case 2:
                acc += 20;
                if ((i & 1) != 0) {
                    continue;
                }
                acc += 200;
                break;
            case 4:
                acc += 40;
                break;
        }

        acc += 7;
    }

    printf("%d\n", acc);
    return 0;
}
