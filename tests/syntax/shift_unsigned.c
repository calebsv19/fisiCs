// Ensure unsigned right shift folds logically.
enum E {
    SHIFT_OK = ((unsigned)0x80000000u >> 1) == 0x40000000u ? 1 : -1
};

int arr[SHIFT_OK];
