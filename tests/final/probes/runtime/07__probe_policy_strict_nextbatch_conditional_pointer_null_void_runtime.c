#include <stdio.h>

static int read_const_slot(const void *base, int index) {
    const int *values = (const int*)base;
    return values[index];
}

int main(void) {
    int values[4] = {3, 5, 8, 13};
    int *p = &values[1];
    int *z = 0;
    const int *cp = &values[2];

    int ok_selected_ptr = ((1 ? p : z) == &values[1]);
    int ok_selected_null = ((0 ? p : z) == 0);

    const void *cv = (1 ? (const void*)cp : (const void*)0);
    const int *roundtrip = (const int*)cv;
    int ok_roundtrip_ptr = (roundtrip == &values[2]);
    int ok_roundtrip_value = read_const_slot(cv, 0);

    void *vp = (0 ? (void*)z : (void*)values);
    int ok_void_base = ((int*)vp == values);

    printf("%d %d %d %d %d\n",
           ok_selected_ptr,
           ok_selected_null,
           ok_roundtrip_ptr,
           ok_roundtrip_value,
           ok_void_base);
    return 0;
}
