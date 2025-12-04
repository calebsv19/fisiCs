struct S {
    int a;
    char b;
};

union U {
    int x;
    char y;
};

int arr[sizeof(struct S)];
int arr2[sizeof(union U)];

int f(int v) {
    switch (v) {
        case sizeof(struct S):
            return arr[0];
        case sizeof(union U):
            return arr2[0];
        default:
            return 0;
    }
}
