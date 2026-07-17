#include <stdio.h>

int main(void) {
    int selectors[] = {1, 0, 2, 3, 1, 4, 2};
    int acc = 0;
    int i;

    for (i = 0; i < 7; ++i) {
        int j;

        for (j = 0; j < 3; ++j) {
            switch ((selectors[i] + j) % 5) {
                case 0:
                    acc += 3 + i + j;
                    continue;

                case 1:
                    acc += 10;
                    goto inner_tail;

                case 2:
                    acc += 20 + j;
                    break;

                default:
                    acc += 30 + selectors[i];
                    if (j == 1) {
                        continue;
                    }

                case 4:
                    acc += 40 + i;
                    break;
            }

            acc += 5;
inner_tail:
            acc += 2 + j;
        }

        acc += 100 + i;
    }

    printf("%d\n", acc);
    return 0;
}
