#include <stdio.h>

int main(void) {
    int selectors[] = {0, 2, 1, 3, 2};
    int acc = 0;
    int i;

    for (i = 0; i < 5; ++i) {
        switch (selectors[i]) {
            case 0: {
                int base = i + 4;
                if (i != 0) {
                    acc += 1;
                }
case_zero_body:
                acc += base;
            }

            case 2: {
                int base = i + 20;
                acc += base;
                if (i == 4) {
                    goto after_switch;
                }
                break;
            }

            case 1: {
                int j;
                for (j = 0; j < 3; ++j) {
                    int local = i + j;
                    acc += local;
                    if (j == 1) {
                        continue;
                    }
                    acc += 10 + j;
                }
                break;
            }

            default: {
                int base = i + 30;
                acc += base;
                continue;
            }
        }

after_switch:
        acc += 100 + i;
    }

    printf("%d\n", acc);
    return 0;
}
