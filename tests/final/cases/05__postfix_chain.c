struct S { int a[2]; };
int main(void) {
    struct S s = {{1,2}};
    int x = (s).a[1];
    return x;
}
