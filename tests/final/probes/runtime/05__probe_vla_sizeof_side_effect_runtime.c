int main(void) {
    int n = 3;
    int values[n++];
    int size = (int)sizeof(values);
    return (n == 4 && size == (int)(3 * sizeof(int))) ? 0 : 1;
}
