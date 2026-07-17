#include <stdio.h>

int main(void) {
    int values[6] = {1, 3, 5, 7, 9, 11};
    int *begin = values;
    int *middle = values + 3;
    int *end = values + 6;
    int *selected_end = (1 ? end : middle);
    int *selected_mid = (0 ? end : (1 ? middle : begin));
    long span = (long)(selected_end - selected_mid);
    int within = (selected_mid < selected_end) && (begin <= selected_mid);
    int eq_one_past = (0 ? middle : selected_end) == values + 6;
    int deref_safe = *((selected_end > begin) ? selected_end - 1 : begin);

    printf("%ld %d %d %d\n",
           span,
           within,
           eq_one_past,
           deref_safe);
    return 0;
}
