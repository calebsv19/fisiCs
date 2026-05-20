#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -3:
            return 7;
        case 0:
            return 11;
        case 9:
            return 13;
        default:
            return 100;
    }
}

int main(void) {
    int values[5] = {-3, -2, 0, 9, 4};
    int i;
    int sum = 0;

    for (i = 0; i < 5; ++i) {
        switch (dispatch(values[i])) {
            case 7:
                sum += 1;
                break;
            case 11:
                sum += 10;
                break;
            case 13:
                sum += 100;
                break;
            default:
                sum += 1000;
                break;
        }
    }

    printf("%d\n", sum);
    return 0;
}
