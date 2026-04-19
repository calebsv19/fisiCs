enum ValueSetA {
    A0 = 1,
    A1 = A0 + 3,
    A2 = (A1 * 2) - 1,
    A3 = (A2 << 1) ^ 7,
    A4 = (A3 / 3) + 19
};

enum ValueSetB {
    B0 = A4 + 5,
    B1 = (B0 * 4) - A2,
    B2 = (B1 % 9) + 22,
    B3 = (B2 > 20 ? B2 + 1 : 21),
    B4 = B3 + A1 + A0
};

_Static_assert(A0 == 1, "a0");
_Static_assert(A1 == 4, "a1");
_Static_assert(A2 == 7, "a2");
_Static_assert(B3 >= 20, "b3");
_Static_assert((B4 - A1) > 0, "b4");

int table0[(A1 + A2) > 0 ? 8 : -1];
int table1[(B0 - A0) > 0 ? 16 : -1];
int table2[(B4 ^ A4) >= 0 ? 4 : -1];

int select_value(int x) {
    switch (x) {
        case A0: return A1;
        case A1: return A2;
        case A2: return A3;
        case A3: return A4;
        case B0: return B1;
        case B1: return B2;
        case B2: return B3;
        case B3: return B4;
        default: return table0[0] + table1[0] + table2[0];
    }
}

int main(void) {
    return select_value(B2);
}
