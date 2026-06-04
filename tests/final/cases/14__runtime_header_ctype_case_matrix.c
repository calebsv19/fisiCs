#include <ctype.h>
#include <stdio.h>

int main(void) {
    char text[] = "AbCz09";
    int i;
    int lower_sum = 0;
    int upper_sum = 0;

    for (i = 0; text[i] != '\0'; ++i) {
        unsigned char ch = (unsigned char)text[i];
        lower_sum += tolower(ch);
        upper_sum += toupper(ch);
    }

    printf("ctype-case lower=%d upper=%d delta=%d\n", lower_sum, upper_sum, lower_sum - upper_sum);
    return lower_sum == 521 && upper_sum == 393 && lower_sum - upper_sum == 128 ? 0 : 1;
}
