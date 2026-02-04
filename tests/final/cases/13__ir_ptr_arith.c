int main(void) {
    int arr[3] = {0};
    int *p = arr;
    int diff = (int)((p + 2) - p);
    return diff;
}
