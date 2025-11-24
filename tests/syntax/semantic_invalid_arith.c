struct Vector {
    int x;
    int y;
};

int main(void) {
    struct Vector v = {1, 2};
    _Bool flag = 1;
    int value = v + 3;
    int mask = flag & 1.0;
    return value + mask;
}
