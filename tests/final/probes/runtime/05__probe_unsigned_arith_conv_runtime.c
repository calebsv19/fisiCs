int main(void) {
    unsigned u = 1u;
    int n = -2;
    unsigned r = u + n;
    return r == (unsigned)-1 ? 0 : 1;
}
