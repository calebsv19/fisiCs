// Basic alignas/_Alignas parsing and alignment queries.
#include <stddef.h>

alignas(16) struct AlignedStruct {
    int x;
};

alignas(32) int g_aligned;

int arr1[_Alignof(struct AlignedStruct) == 16 ? 1 : -1];
int arr2[_Alignof(g_aligned) == 32 ? 1 : -1];

int main(void) {
    alignas(8) int local = 0;
    struct AlignedStruct s = { .x = g_aligned };
    int arr[2] = { local, s.x };
    int arr3[_Alignof(local) == 8 ? 1 : -1];
    return arr[0] + arr[1] + arr1[0] + arr2[0] + arr3[0];
}
