int add(int a, int b) { return a + b; }
int main(void) {
    int arr[3] = {1,2,3};
    int *p = arr;
    int (*fp)(int,int) = add;
    return p[0] + fp(1,2);
}
