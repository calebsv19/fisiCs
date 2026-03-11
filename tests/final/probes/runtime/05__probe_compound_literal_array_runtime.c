int main(void) {
    int *p = (int[]){1, 2, 3, 4};
    return p[0] + p[3] == 5 ? 0 : 1;
}
