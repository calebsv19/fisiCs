struct __attribute__((aligned(16))) A {
    int x;
};

struct __attribute__((packed)) B {
    int x;
    char y;
};

int arr1[sizeof(struct A) == 16 ? 1 : -1];
int arr2[sizeof(struct B) == 5 ? 1 : -1];

int main(void) {
    return arr1[0] + arr2[0];
}
