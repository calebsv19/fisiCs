int main(void) {
    int y = sizeof(int[2]);
    int z = sizeof((const int[]){1, 2});
    return y + z;
}
