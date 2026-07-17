#include <stdio.h>

int main(void) {
    int values[5] = {1, 1, 2, 3, 5};
    int *begin = values;
    int *cursor = (1 ? values + 3 : values + 1);
    int *end = values + 5;
    int *chosen = (0 ? cursor : end);
    const void *cv_end = (1 ? (const void*)end : (const void*)values);
    const int *const_end = (const int*)cv_end;

    int ok_order0 = (begin < cursor);
    int ok_order1 = (cursor < end);
    int ok_chosen = (chosen == end);
    int ok_const_end = (const_end == end);
    int span = (int)(end - begin);
    int tail = (int)(chosen - cursor);

    printf("%d %d %d %d %d %d\n",
           ok_order0,
           ok_order1,
           ok_chosen,
           ok_const_end,
           span,
           tail);
    return 0;
}
