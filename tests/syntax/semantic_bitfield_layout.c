// Bitfield layout and zero-width realignment.
struct BF {
    unsigned a : 3;
    unsigned b : 10;
    unsigned c : 3;
    unsigned d : 16; // spills to next storage unit
};

struct ZW {
    unsigned a : 4;
    unsigned : 0; // realign to next unit
    unsigned b : 4;
};

int arr0[sizeof(struct BF) == 4 ? 1 : -1];
int arr1[_Alignof(struct BF) == 4 ? 1 : -1];
int arr2[sizeof(struct ZW) == 8 ? 1 : -1];
int arr3[_Alignof(struct ZW) == 4 ? 1 : -1];

int main(void) {
    struct BF bf = { .a = 1, .b = 2, .c = 3, .d = 4 };
    struct ZW zw = { .a = 5, .b = 6 };
    return bf.a + bf.b + bf.c + bf.d + zw.a + zw.b +
           arr0[0] + arr1[0] + arr2[0] + arr3[0];
}
