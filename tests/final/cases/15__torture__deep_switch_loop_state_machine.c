#include <stdio.h>

int main(void) {
    int state = 7;
    int sum = 0;

    for (int i = 0; i < 140; ++i) {
        int key = (i * 3 + state + (sum & 7)) % 10;
        switch (key) {
            case 0:
                state = (state + 1) % 37;
                sum += state;
                continue;
            case 1:
            case 2:
                state = (state * 2 + key + i) % 41;
                break;
            case 3:
                sum += i;
                /* fallthrough */
            case 4:
                state = (state ^ (i & 15)) % 43;
                break;
            case 5:
                if ((i & 1) == 0) {
                    state = (state + 5) % 47;
                    continue;
                }
                state = (state + 44) % 47;
                break;
            case 6:
                state = (state + (sum % 23)) % 53;
                break;
            case 7:
                if ((sum % 11) == 0) {
                    sum += state;
                    break;
                }
                state = (state + 11) % 59;
                continue;
            case 8:
                state = (state * state + i) % 61;
                break;
            default:
                sum += (state % 9);
                break;
        }
        sum += (state & 31);
    }

    printf("%d %d\n", state, sum);
    return 0;
}
