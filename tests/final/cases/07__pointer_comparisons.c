int main(void) {
    int arr[2] = {0};
    int *p = arr;
    int *q = arr + 1;
    int ok = (p != 0) && (q > p);
    return ok;
}
