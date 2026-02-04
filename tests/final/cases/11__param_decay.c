int takes_arr(int a[]) { return a[0]; }
int takes_fn(int (*fn)(int)) { return fn(1); }
int inc(int x) { return x + 1; }
int main(void) {
    int arr[2] = {1, 2};
    return takes_arr(arr) + takes_fn(inc);
}
