// Pointer arithmetic on array names should decay correctly and pointer
// differences should use the element size.
#include <stddef.h>

int main(void) {
    int arr[3] = {1, 2, 3};
    int *p = arr + 1;
    ptrdiff_t d = &arr[2] - p;
    return (int)d;
}
