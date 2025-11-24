int main(void) {
    int data[5] = {1, 2, 3, 4, 5};
    int *a = &data[4];
    int *b = &data[1];
    long int delta = a - b;
    return (int)delta;
}
