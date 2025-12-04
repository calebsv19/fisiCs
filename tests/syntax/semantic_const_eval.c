int arr1[1 + 2 * 3];
int arr2[(1 || 0) ? 4 : 2];
int arr3[(1 && 0) || (5 > 3)];
int arr4[(~0u >> 30) + (3 << 2)];

enum Numbers {
    ONE = 1,
    TWO = ONE + 1,
    FOUR = (TWO << 1)
};

int arr5[FOUR];
int invalid[(int)foo];

int main(void) {
    return arr1[0] + arr2[0] + arr3[0] + arr4[0] + arr5[0];
}
