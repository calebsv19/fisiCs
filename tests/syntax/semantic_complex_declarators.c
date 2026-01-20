int *retptr(int *p) { return p; }

int (*make(void))[3] {
    static int arr[3] = {1, 2, 3};
    return &arr;
}

int main(void) {
    int a[2][3] = {{1, 2, 3}, {4, 5, 6}};
    int (*p[2])[3] = {&a[0], &a[1]};
    int x = 3;
    int *(*fp)(int *) = retptr;
    int ok = (*p[0])[1] == 2;
    int ok2 = *fp(&x) == 3;
    int ok3 = (*make())[1] == 2;
    return (ok && ok2 && ok3) ? 0 : 1;
}
