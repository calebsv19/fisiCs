#include <stdio.h>

int main(void) {
    int arr[6] = {1, 2, 3, 4, 5, 6};
    int *p = arr;
    int *q = arr + 5;
    long diff = (long)(q - p);

    void *vp = (void*)arr;
    int *from_void = (int*)vp;
    int ok_roundtrip = (from_void == arr);

    int (*ap)[6] = &arr;
    int *from_array_ptr = &(*ap)[0];
    int ok_decay = (from_array_ptr == arr);

    printf("%ld %d %d\n", diff, ok_roundtrip, ok_decay);
    return 0;
}
