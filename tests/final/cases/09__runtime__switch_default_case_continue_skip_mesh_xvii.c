#include <stdio.h>

int main(void) {
    int selectors[] = {1, 3, 0, 2, 4, 1};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        switch (selectors[i]) {
            case 1:
                acc += 10;
                switch (i & 1) {
                    case 0:
                        acc += 100;
                        break;
                    default:
                        acc += 200;
                        break;
                }
                break;
            default:
                acc += 1;
            case 2:
                acc += 20;
                if (i == 3 || i == 4) {
                    continue;
                }
                acc += 2000;
                break;
            case 4:
                acc += 40;
                break;
        }

        acc += i + 3;
    }

    printf("%d\n", acc);
    return 0;
}
