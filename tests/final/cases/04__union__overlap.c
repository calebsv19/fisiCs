union U {
    int i;
    double d;
};

int main(void) {
    union U u;
    long off_i = (long)((char*)&u.i - (char*)&u);
    long off_d = (long)((char*)&u.d - (char*)&u);

    if (off_i != 0 || off_d != 0) {
        return 1;
    }
    if (sizeof(u) < sizeof(int) || sizeof(u) < sizeof(double)) {
        return 2;
    }
    return 0;
}
