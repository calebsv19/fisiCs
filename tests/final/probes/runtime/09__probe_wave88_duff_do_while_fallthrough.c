#include <stdio.h>

static int duff_digits(int count) {
    int value = 0;
    int rounds = (count + 3) / 4;

    switch (count % 4) {
        case 0:
            do {
                value = value * 10 + 1;
        case 3:
                value = value * 10 + 2;
        case 2:
                value = value * 10 + 3;
        case 1:
                value = value * 10 + 4;
            } while (--rounds > 0);
    }

    return value;
}

int main(void) {
    printf("%d %d %d\n", duff_digits(1), duff_digits(5), duff_digits(8));
    return 0;
}
