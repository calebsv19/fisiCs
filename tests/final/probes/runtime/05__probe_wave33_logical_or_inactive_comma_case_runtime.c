#include <stdio.h>

int main(void) {
    int result = 0;
    switch (1) {
        case 1 || (0, 1):
            result = 11;
            break;
    }
    printf("%d\n", result);
    return 0;
}
