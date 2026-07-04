#include <stdio.h>

int main(void) {
    int selectors[] = {2, 5, 4, 7, 2, 9};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        switch (selectors[i]) {
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
            default:
                acc += 1;
                break;
        }

        acc += 7;
    }

    printf("%d\n", acc);
    return 0;
}
