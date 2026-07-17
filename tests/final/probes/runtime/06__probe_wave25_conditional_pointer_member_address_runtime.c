#include <stdio.h>

struct Wave25Payload {
    int value;
    int adjust;
};

int main(void) {
    struct Wave25Payload left = {5, 7};
    struct Wave25Payload right = {29, 31};
    int pick = right.value > left.value;
    int *value = &(pick ? &right : &left)->value;
    int *adjust = &(pick ? &right : &left)->adjust;

    *value += *adjust;
    *adjust += *value;
    printf("%d %d\n", *value, *adjust);
    return 0;
}
