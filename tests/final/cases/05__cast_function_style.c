typedef int T;
int main(void) {
    int x = 3;
    int y = (T)x;
    int z = T(x);
    return y + z;
}
