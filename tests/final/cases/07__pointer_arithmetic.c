int main(void) {
    int arr[5] = {0};
    int *p = arr;
    int *q = p + 2;
    int diff = (int)(q - p);
    return diff;
}
