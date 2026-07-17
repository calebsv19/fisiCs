#include <stdio.h>

int main(void) {
    int values[5] = {4, 6, 9, 13, 18};
    int *p = &values[2];
    const int *cp = &values[4];

    const int *selected_const = (1 ? p : cp);
    const int *selected_null = (0 ? p : 0);
    const void *cv = (1 ? (const void*)cp : (const void*)0);
    void *vp = (0 ? (void*)p : (void*)0);

    long diff = (long)((1 ? &values[4] : &values[0]) -
                       (0 ? &values[4] : &values[1]));

    printf("%d %d %d %ld %d\n",
           selected_const == &values[2],
           selected_null == 0,
           *(const int*)cv,
           diff,
           vp == 0);
    return 0;
}
