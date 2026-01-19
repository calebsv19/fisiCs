int main(void) {
    int arr[2] = {1, 2};
    int (*arrp)[2] = &arr;
    return arrp[0][1];
}
